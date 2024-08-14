/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _SYS_ATCSMU_H
#define _SYS_ATCSMU_H

#include <sbi/sbi_types.h>

/* clang-format off */
#define PCS_WAKE_MSIP_OFFSET	29

#define PCS0_SCRATCH_OFFSET	0x84
#define PCSm_SCRATCH_OFFSET(n) ((n + 3) * 0x20 + PCS0_SCRATCH_OFFSET)

#define PCS0_WE_OFFSET		0x90
#define PCSm_WE_OFFSET(i)	((i + 3) * 0x20 + PCS0_WE_OFFSET)

#define PCS0_CTL_OFFSET		0x94
#define PCSm_CTL_OFFSET(i)	((i + 3) * 0x20 + PCS0_CTL_OFFSET)
#define PCS_CTL_CMD_SHIFT	0
#define PCS_CTL_PARAM_SHIFT	3
#define SLEEP_CMD		0x3
#define WAKEUP_CMD		(0x0 | (1 << PCS_CTL_PARAM_SHIFT))
#define LIGHTSLEEP_MODE		0
#define DEEPSLEEP_MODE		1
#define LIGHT_SLEEP_CMD		(SLEEP_CMD | (LIGHTSLEEP_MODE << PCS_CTL_PARAM_SHIFT))
#define DEEP_SLEEP_CMD		(SLEEP_CMD | (DEEPSLEEP_MODE << PCS_CTL_PARAM_SHIFT))
#define LIGHT_SLEEP_STATUS	0x0
#define DEEP_SLEEP_STATUS	0x10

#define PCS0_CFG_OFFSET			0x80
#define PCSm_CFG_OFFSET(i)		((i + 3) * 0x20 + PCS0_CFG_OFFSET)
#define PCS_CFG_LIGHT_SLEEP_SHIFT	2
#define PCS_CFG_LIGHT_SLEEP		(1 << PCS_CFG_LIGHT_SLEEP_SHIFT)
#define PCS_CFG_DEEP_SLEEP_SHIFT	3
#define PCS_CFG_DEEP_SLEEP		(1 << PCS_CFG_DEEP_SLEEP_SHIFT)

#define RESET_VEC_LO_OFFSET	0x50
#define RESET_VEC_HI_OFFSET	0x60
#define RESET_VEC_8CORE_OFFSET	0x1a0
#define HARTn_RESET_VEC_LO(n)	(RESET_VEC_LO_OFFSET + \
			        ((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + \
			        ((n) * 0x4))
#define HARTn_RESET_VEC_HI(n)	(RESET_VEC_HI_OFFSET + \
			        ((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + \
			        ((n) * 0x4))

#define PCS0_STATUS_OFFSET	0x98
#define PCSm_STATUS_OFFSET(i)	((i + 3) * 0x20 + PCS0_STATUS_OFFSET)
// PD* mask
#define PD_TYPE_MASK		0x7
#define PD_STATUS_MASK		0xf8
// PCS_STATUS[0:2]: pd_type
#define ACTIVE			0
#define RESET			1
#define SLEEP			2
// PCS_STATUS[7:3]: pd_status for sleep mode
#define LIGHT_SLEEP_STATUS	0x0
#define DEEP_SLEEP_STATUS	0x10

#define GET_PD_TYPE(val)	((val) & PD_TYPE_MASK)
#define GET_PD_STATUS(val)	(((val) & PD_STATUS_MASK) >> 3)

#define PCS_MAX_NR  8
#define FLASH_BASE  0x80000000ULL

/* Andes AE350 sleep type */
#define SBI_SUSP_AE350_LIGHT_SLEEP                     0x80000001
#define SBI_SUSP_AE350_DEEP_SLEEP                      0x80000002

/* clang-format on */

struct smu_data {
	unsigned long addr;
};

int smu_set_wakeup_events(struct smu_data *smu, u32 events, u32 hartid);
bool smu_support_sleep_mode(struct smu_data *smu, u32 sleep_mode, u32 hartid);
int smu_set_command(struct smu_data *smu, u32 pcs_ctl, u32 hartid);
int smu_set_reset_vector(struct smu_data *smu, ulong wakeup_addr, u32 hartid);
u32 smu_get_sleep_type(struct smu_data *smu, u32 hartid);
int smu_check_pcs_status(struct smu_data *smu, u32 sleep_status, u32 hartid);

#endif /* _SYS_ATCSMU_H */
