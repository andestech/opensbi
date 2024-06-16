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

#if __riscv_xlen == 64
#define SHMEM_PHYS_ADDR(_hi, _lo) (_lo)
#elif __riscv_xlen == 32
#define SHMEM_PHYS_ADDR(_hi, _lo) (((u64)(_hi) << 32) | (_lo))
#else
#error "Undefined XLEN"
#endif

#define INVALID_ADDR		(-1U)
#define IS_SHMEM_ADDR_VALID(ms) \
		((ms)->shmem.shmem_addr_lo != INVALID_ADDR && \
		 (ms)->shmem.shmem_addr_hi != INVALID_ADDR)

/** Get hart shared memory base address */
static inline void *hart_shmem_base(struct mpxy_state *rs)
{
	return (void *)(unsigned long)SHMEM_PHYS_ADDR(rs->shmem.shmem_addr_hi,
						rs->shmem.shmem_addr_lo);
}

/** SPD TEE MPXY Message IDs */
enum mpxy_opteed_message_id {
	OPTEED_MSG_COMMUNICATE = 0x01,
	OPTEED_MSG_COMPLETE = 0x02,
};

struct abi_entry_vectors {
	uint32_t yield_abi_entry;
	uint32_t fast_abi_entry;
};

struct abi_entry_vectors *entry_vector_table = NULL;

#define ABI_ENTRY_TYPE_FAST		1
#define ABI_ENTRY_TYPE_YIELD		0
#define FUNCID_TYPE_SHIFT		31
#define FUNCID_TYPE_MASK		0x1
#define GET_ABI_ENTRY_TYPE(id)		(((id) >> FUNCID_TYPE_SHIFT) & \
					 FUNCID_TYPE_MASK)

/* Defined in optee_os/core/arch/riscv/include/tee/teeabi_opteed.h */
#define TEEABI_OPTEED_RETURN_CALL_DONE 0xBE000000

static char opteed_domain_name[64];

static int opteed_domain_setup(void *fdt, int nodeoff, const struct fdt_match *match)
{
	const u32 *prop_instance;
	int len, offset;

	prop_instance = fdt_getprop(fdt, nodeoff, "opensbi-domain-instance", &len);
	if (!prop_instance || len < 4)
		return SBI_EINVAL;

	offset = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*prop_instance));
	if (offset < 0)
		return SBI_EINVAL;

	strncpy(opteed_domain_name, fdt_get_name(fdt, offset, NULL),
		sizeof(opteed_domain_name));
	opteed_domain_name[sizeof(opteed_domain_name) - 1] = '\0';

	return 0;
}

static struct sbi_domain *__get_tdomain(void)
{
	int i;
	struct sbi_domain *dom = NULL;
	sbi_domain_for_each(i, dom)
	{
		if (!sbi_strcmp(dom->name, opteed_domain_name)) {
			return dom;
		}
	}

	return NULL;
}

static struct sbi_domain *__get_udomain(void)
{
	int i;
	struct sbi_domain *dom = NULL;
	sbi_domain_for_each(i, dom)
	{
		if (!sbi_strcmp(dom->name, "untrusted-domain")) {
			return dom;
		}
	}

	return NULL;
}

static int sbi_ecall_tee_domain_enter(unsigned long entry_point)
{
	int i;
	struct sbi_domain *dom, *tdom = NULL;
	sbi_domain_for_each(i, dom)
	{
		if (!sbi_strcmp(dom->name, opteed_domain_name)) {
			tdom = dom;
			break;
		}
	}

	if (tdom) {
		sbi_domain_context_set_mepc(tdom, entry_point);
		sbi_domain_context_enter(tdom);
	}
	return 0;
}

static int sbi_ecall_tee_domain_exit(void)
{
	sbi_domain_context_exit();
	return 0;
}

static int mpxy_opteed_send_message(struct sbi_mpxy_channel *channel,
			    u32 msg_id, void *msgbuf, u32 msg_len,
			    void *respbuf, u32 resp_max_len,
			    unsigned long *resp_len)
{
	u32 hartidx = sbi_hartid_to_hartindex(current_hartid());
	struct mpxy_state *ms;
	void *shmem_base;
	u32 funcid_type;

	if (msg_id == OPTEED_MSG_COMMUNICATE) {
		/* Get per-hart MPXY share memory with tdomain */
		ms = sbi_hartindex_to_domain_rs(hartidx, __get_tdomain());
		shmem_base = hart_shmem_base(ms);

		if(!IS_SHMEM_ADDR_VALID(ms)) {
			sbi_printf("hart%d trusted domain MPXY shared memory is not valid\n",
				   current_hartid());
			return SBI_EINVAL;
		}

		sbi_memcpy(shmem_base, msgbuf, msg_len);

		funcid_type = GET_ABI_ENTRY_TYPE(((ulong *)shmem_base)[0]);
		sbi_ecall_tee_domain_enter((funcid_type == ABI_ENTRY_TYPE_FAST) ?
					   (ulong)&entry_vector_table->fast_abi_entry :
					   (ulong)&entry_vector_table->yield_abi_entry);
	} else if (msg_id == OPTEED_MSG_COMPLETE) {
		/* Get per-hart MPXY share memory with udomain */
		ms = sbi_hartindex_to_domain_rs(hartidx, __get_udomain());

		if(!IS_SHMEM_ADDR_VALID(ms)) {
			if (((ulong *)msgbuf)[0] == TEEABI_OPTEED_RETURN_CALL_DONE) {
				/* RETURN_INIT_DONE */
				entry_vector_table = (struct abi_entry_vectors *)(((ulong *)msgbuf)[1]);
				sbi_printf("Registered OP-TEE entry table: %#lx\n", (ulong)entry_vector_table);
			}
		} else {
			/* tx has a0~a4. Just skip a0 and copy a1~a4 here */
			sbi_memcpy(hart_shmem_base(ms),
				   &(((ulong *)msgbuf)[1]),
				   msg_len - sizeof(ulong));
			*resp_len = msg_len - sizeof(ulong);
		}

		sbi_ecall_tee_domain_exit();
	} else {
		sbi_printf("%s: message id %d not supported by channel%d\n",
			   __func__, msg_id, channel->channel_id);
		return SBI_EINVAL;
	}

	return 0;
}

static int mpxy_opteed_init(void *fdt, int nodeoff,
			  const struct fdt_match *match)
{
	struct sbi_mpxy_channel *channel;
	const fdt32_t *val;
	u32 channel_id;
	int rc, len;

	/* Allocate context for MPXY channel */
	channel = sbi_zalloc(sizeof(*channel));
	if (!channel)
		return SBI_ENOMEM;

	/* Setup domain for OP-TEE dispatcher */
	rc = opteed_domain_setup(fdt, nodeoff, match);
	if (rc) {
		sbi_free(channel);
		return 0;
	}

	val = fdt_getprop(fdt, nodeoff, "riscv,sbi-mpxy-channel-id", &len);
	if (len > 0 && val)
		channel_id = fdt32_to_cpu(*val);
	else
		sbi_panic("Failed to get riscv,sbi-mpxy-channel-id");

	channel->channel_id = channel_id;
	channel->send_message = mpxy_opteed_send_message;
	channel->attrs.msg_proto_id = SBI_MPXY_MSGPROTO_TEE_ID;
	channel->attrs.msg_data_maxlen = PAGE_SIZE;

	rc = sbi_mpxy_register_channel(channel);
	if (rc) {
		sbi_free(channel);
		return rc;
	}

	return 0;
}

static const struct fdt_match mpxy_opteed_match[] = {
	{ .compatible = "riscv,sbi-mpxy-opteed", .data = NULL },
	{},
};

struct fdt_mpxy fdt_mpxy_opteed = {
	.match_table = mpxy_opteed_match,
	.init = mpxy_opteed_init,
};
