/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Intel Corporation. All rights reserved.
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_heap.h>
#include <sbi/sbi_mpxy.h>
#include <libfdt.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mpxy/fdt_mpxy.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_console.h>

#define SBI_MPXY_STMM_MSG_CHANNEL_ID	(0x0)
#define SBI_MPXY_STMM_MSG_DATA_MAX_SIZE	(0x200000)    /* bytes */

struct mm_cpu_info {
	u64 mpidr;
	u32 linear_id;
	u32 flags;
};

struct mm_boot_info {
	u64 mm_mem_base;
	u64 mm_mem_limit;
	u64 mm_image_base;
	u64 mm_stack_base;
	u64 mm_heap_base;
	u64 mm_ns_comm_buf_base;
	u64 mm_shared_buf_base;
	u64 mm_image_size;
	u64 mm_pcpu_stack_size;
	u64 mm_heap_size;
	u64 mm_ns_comm_buf_size;
	u64 mm_shared_buf_size;
	u32 num_mem_region;
	u32 num_cpus;
	struct mm_cpu_info *cpu_info;
};

struct mm_boot_args {
	struct mm_boot_info boot_info;
	struct mm_cpu_info cpu_info[SBI_HARTMASK_MAX_BITS];
};

static struct {
	struct sbi_domain *udomain;
	struct sbi_domain *tdomain;
} mm_domains;

static struct sbi_domain *__get_domain(char* name)
{
	int i;
	struct sbi_domain *dom = NULL;
	sbi_domain_for_each(i, dom)
	{
		if (!sbi_strcmp(dom->name, name)) {
			return dom;
		}
	}
	return NULL;
}

int mm_srv_setup(void *fdt, int nodeoff, const struct fdt_match *match)
{
	const u32 *prop_instance, *prop_value;
	u64 base64, size64;
	char name[64];
	int i, len, offset;

	struct mm_boot_args *boot_args = NULL;
	struct mm_boot_info *boot_info = NULL;
	prop_instance = fdt_getprop(fdt, nodeoff, "tdomain-instance", &len);
	if (!prop_instance || len < 4) {
		return SBI_EINVAL;
	}
	offset = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*prop_instance));
	if (offset < 0) {
		return SBI_EINVAL;
	}
	sbi_memset(name, 0, 64);
	strncpy(name, fdt_get_name(fdt, offset, NULL), sizeof(name));
	mm_domains.tdomain = __get_domain(name);
	if (NULL == mm_domains.tdomain)
		return SBI_EINVAL;

	prop_instance = fdt_getprop(fdt, nodeoff, "udomain-instance", &len);
	if (!prop_instance || len < 4) {
		return SBI_EINVAL;
	}
	offset = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*prop_instance));
	if (offset < 0) {
		return SBI_EINVAL;
	}
	sbi_memset(name, 0, 64);
	strncpy(name, fdt_get_name(fdt, offset, NULL), sizeof(name));
	mm_domains.udomain = __get_domain(name);
	if (NULL == mm_domains.udomain)
		return SBI_EINVAL;

	boot_args = (void *)mm_domains.tdomain->next_arg1;
	boot_info = &boot_args->boot_info;
	prop_value = fdt_getprop(fdt, nodeoff, "num-regions", &len);
	if (!prop_value || len < 4)
		return SBI_EINVAL;
	boot_info->num_mem_region = (unsigned long)fdt32_to_cpu(*prop_value);
	prop_value = fdt_getprop(fdt, nodeoff, "memory-reg", &len);
	if (!prop_value || len < 16)
		return SBI_EINVAL;
	base64 = fdt32_to_cpu(prop_value[0]);
	base64 = (base64 << 32) | fdt32_to_cpu(prop_value[1]);
	size64 = fdt32_to_cpu(prop_value[2]);
	size64 = (size64 << 32) | fdt32_to_cpu(prop_value[3]);
	boot_info->mm_mem_base	= base64;
	boot_info->mm_mem_limit	= base64 + size64;
	prop_value = fdt_getprop(fdt, nodeoff, "image-reg", &len);
	if (!prop_value || len < 16)
		return SBI_EINVAL;
	base64 = fdt32_to_cpu(prop_value[0]);
	base64 = (base64 << 32) | fdt32_to_cpu(prop_value[1]);
	size64 = fdt32_to_cpu(prop_value[2]);
	size64 = (size64 << 32) | fdt32_to_cpu(prop_value[3]);
	boot_info->mm_image_base	= base64;
	boot_info->mm_image_size	= size64;

	prop_value = fdt_getprop(fdt, nodeoff, "heap-reg", &len);
	if (!prop_value || len < 16)
		return SBI_EINVAL;
	base64 = fdt32_to_cpu(prop_value[0]);
	base64 = (base64 << 32) | fdt32_to_cpu(prop_value[1]);
	size64 = fdt32_to_cpu(prop_value[2]);
	size64 = (size64 << 32) | fdt32_to_cpu(prop_value[3]);
	boot_info->mm_heap_base	= base64;
	boot_info->mm_heap_size	= size64;
	prop_value = fdt_getprop(fdt, nodeoff, "stack-reg", &len);
	if (!prop_value || len < 16)
		return SBI_EINVAL;
	base64 = fdt32_to_cpu(prop_value[0]);
	base64 = (base64 << 32) | fdt32_to_cpu(prop_value[1]);
	size64 = fdt32_to_cpu(prop_value[2]);
	size64 = (size64 << 32) | fdt32_to_cpu(prop_value[3]);
	boot_info->mm_stack_base	= base64 + size64 -1;

	prop_value = fdt_getprop(fdt, nodeoff, "pcpu-stack-size", &len);
	if (!prop_value || len < 4)
		return SBI_EINVAL;
	boot_info->mm_pcpu_stack_size = (unsigned long)fdt32_to_cpu(*prop_value);
	prop_value = fdt_getprop(fdt, nodeoff, "shared-buf", &len);
	if (!prop_value || len < 16)
		return SBI_EINVAL;
	base64 = fdt32_to_cpu(prop_value[0]);
	base64 = (base64 << 32) | fdt32_to_cpu(prop_value[1]);
	size64 = fdt32_to_cpu(prop_value[2]);
	size64 = (size64 << 32) | fdt32_to_cpu(prop_value[3]);
	boot_info->mm_shared_buf_base	= base64;
	boot_info->mm_shared_buf_size	= size64;

	prop_value = fdt_getprop(fdt, nodeoff, "ns-comm-buf", &len);
	if (!prop_value || len < 16)
		return SBI_EINVAL;
	base64 = fdt32_to_cpu(prop_value[0]);
	base64 = (base64 << 32) | fdt32_to_cpu(prop_value[1]);
	size64 = fdt32_to_cpu(prop_value[2]);
	size64 = (size64 << 32) | fdt32_to_cpu(prop_value[3]);
	boot_info->mm_ns_comm_buf_base	= base64;
	boot_info->mm_ns_comm_buf_size	= size64;
	boot_info->num_cpus = 0;
	sbi_hartmask_for_each_hartindex(i, mm_domains.tdomain->possible_harts) {
		boot_args->cpu_info[i].linear_id = sbi_hartindex_to_hartid(i);
		boot_args->cpu_info[i].flags = 0;
		boot_info->num_cpus += 1;
	}
	boot_info->cpu_info = boot_args->cpu_info;

	return 0;
}

static int mm_send_message(struct sbi_mpxy_channel *channel,
				  u32 msg_id, void *msgbuf, u32 msg_len,
			    void *respbuf, u32 resp_max_len,
			    unsigned long *resp_len)
{
	struct mpxy_state *rs;
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	if (dom == mm_domains.tdomain) {
		rs = sbi_hartindex_to_domain_rs(
			sbi_hartid_to_hartindex(current_hartid()), mm_domains.udomain);
		if (rs->shmem.shmem_addr_lo && msgbuf && ((void *)rs->shmem.shmem_addr_lo != msgbuf)) {
			sbi_memcpy((void *)rs->shmem.shmem_addr_lo, msgbuf, msg_len);
		}
		sbi_domain_context_exit();
	} else {
		rs = sbi_hartindex_to_domain_rs(
			sbi_hartid_to_hartindex(current_hartid()), mm_domains.tdomain);
		if (rs->shmem.shmem_addr_lo && msgbuf && ((void *)rs->shmem.shmem_addr_lo != msgbuf)) {
			sbi_memcpy((void *)rs->shmem.shmem_addr_lo, msgbuf, msg_len);
		}

		sbi_domain_context_enter(mm_domains.tdomain);
	}

	return 0;
}

static int mpxy_mm_init(void *fdt, int nodeoff,
			  const struct fdt_match *match)
{
	int rc;
	struct sbi_mpxy_channel *channel;

	/* Allocate context for MPXY channel */
	channel = sbi_zalloc(sizeof(struct sbi_mpxy_channel));
	if (!channel)
		return SBI_ENOMEM;

	/* Setup MM dispatcher */
	rc = mm_srv_setup(fdt, nodeoff, match);
	if (rc) {
		sbi_free(channel);
		return 0;
	}

	channel->channel_id = SBI_MPXY_STMM_MSG_CHANNEL_ID;
	channel->send_message = mm_send_message;
	channel->attrs.msg_proto_id = SBI_MPXY_MSGPROTO_STMM_ID;
	channel->attrs.msg_data_maxlen = SBI_MPXY_STMM_MSG_DATA_MAX_SIZE;

	rc = sbi_mpxy_register_channel(channel);
    if (rc) {
        sbi_free(channel);
  		return rc;
    }

	return 0;
}

static const struct fdt_match mpxy_mm_match[] = {
	{ .compatible = "riscv,sbi-mpxy-mm", .data = NULL }, 
	{},
};

struct fdt_mpxy fdt_mpxy_mm = {
	.match_table = mpxy_mm_match,
	.init = mpxy_mm_init,
};
