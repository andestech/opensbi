/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems, Inc.
 *
 * Author(s):
 *   Himanshu Chauhan <hchauhan@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_ras.h>
#include <sbi/sbi_mpxy.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/ras/fdt_ras.h>
#include <sbi_utils/ras/ras_agent_mpxy.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/ras/riscv_reri_regs.h>
#include <sbi_utils/ras/ghes.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

struct __attribute__((__packed__)) ras_rpmi_resp_hdr {
	int status;
	u32 flags;
	u32 remaining;
	u32 returned;
};

static struct sbi_mpxy_channel ra_mpxy_ch;
static int ra_handle_message(struct sbi_mpxy_channel *channel, u32 msg_id,
			     void *msgbuf, u32 msg_len, void *respbuf,
			     u32 resp_max_len, unsigned long *resp_len);

int ras_agent_mpxy_init(void *fdt, int nodeoff)
{
	int rc, len;
	const fdt32_t *chan_id_p;
	u32 chan_id;

	memset(&ra_mpxy_ch, 0, sizeof(ra_mpxy_ch));

	chan_id_p = fdt_getprop(fdt, nodeoff, "mpxy-chan-id", &len);
	if (!chan_id_p)
		return SBI_ENOENT;

	chan_id = fdt32_to_cpu(*chan_id_p);

	ra_mpxy_ch.channel_id = chan_id;
	ra_mpxy_ch.send_message = ra_handle_message;
	ra_mpxy_ch.get_notification_events = NULL;
	ra_mpxy_ch.switch_eventsstate = NULL;

	rc = sbi_mpxy_register_channel(&ra_mpxy_ch);
	if (rc != SBI_SUCCESS)
		return rc;

	return SBI_SUCCESS;
}

#define BUF_TO_DATA(_msg_buf)		\
	(((uint8_t *)_msg_buf) + sizeof(struct ras_rpmi_resp_hdr))

static int ra_handle_message(struct sbi_mpxy_channel *channel, u32 msg_id,
			     void *msgbuf, u32 msg_len, void *respbuf,
			     u32 resp_max_len, unsigned long *resp_len)
{
	int rc = SBI_SUCCESS;
	int nes, nr;
	u32 src_id;
	struct ras_rpmi_resp_hdr *rhdr;
	u32 *src_list;
	uint8_t *src_desc;
#define MAX_ID_BUF_SZ (sizeof(u32) * MAX_ERR_SRCS)

	switch(msg_id) {
	case RAS_GET_NUM_ERR_SRCS:
		nes = acpi_ghes_get_num_err_srcs();
		*((u32 *)respbuf) = nes;
		*resp_len = sizeof(u32);
		break;

	case RAS_GET_ERR_SRCS_ID_LIST:
		if (!respbuf)
			return -SBI_EINVAL;

		rhdr = (struct ras_rpmi_resp_hdr *)respbuf;
		rhdr->flags = 0;
		src_list = (u32 *)BUF_TO_DATA(respbuf);

		nr = acpi_ghes_get_err_srcs_list(src_list,
						 resp_max_len/sizeof(u32));

		rhdr->status = RPMI_SUCCESS;
		rhdr->returned = nr;
		rhdr->remaining = 0;
		*resp_len = sizeof(*rhdr) + (sizeof(u32) * nr);
		break;

	case RAS_GET_ERR_SRC_DESC:
		rhdr = (struct ras_rpmi_resp_hdr *)respbuf;
		rhdr->flags = 0;
		src_id = *((u32 *)msgbuf);
		src_desc = (uint8_t *)BUF_TO_DATA(respbuf);
		acpi_ghes_get_err_src_desc(src_id, (acpi_ghesv2 *)src_desc);

		rhdr->status = RPMI_SUCCESS;

		rhdr->returned = sizeof(acpi_ghesv2);
		rhdr->remaining = 0;
		*resp_len = sizeof(*rhdr) + sizeof(acpi_ghesv2);
		break;

	default:
		sbi_printf("RAS Agent: Unknown service %u\n", msg_id);
		rc = SBI_ENOENT;
	}

	return rc;
}
