/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _SYS_ATCSMU_H
#define _SYS_ATCSMU_H

#define NormalMode              0
#define LightSleepMode          1
#define DeepSleepMode           2
#define CpuHotplugDeepSleepMode 3

#if __riscv_xlen == 64
#define STORE           sd
#define LOAD            ld
#define LOG_REGBYTES    3
#else
#define STORE           sw
#define LOAD            lw
#define LOG_REGBYTES    2
#endif
#define REGBYTES (1 << LOG_REGBYTES)
#define PUSH(which)             \
    addi sp, sp, -REGBYTES;     \
    STORE which, 0(sp)
#define PUSH_CSR(csr)           \
    addi sp, sp, -REGBYTES;     \
    csrr t0, csr;               \
    STORE t0, 0(sp)
#define POP(which)              \
    LOAD which, 0(sp);          \
    addi sp, sp, REGBYTES
#define POP_CSR(csr)            \
    LOAD t0, 0(sp);             \
    addi sp, sp, REGBYTES;      \
    csrw csr, t0

/*
 * PCS0 --> Always on power domain, includes the JTAG tap and
 *          DMI_AHB bus in NCEJDTM200
 * PCS1 --> Power domain for debug subsystem
 * PCS2 --> Main power domain, includes the system bus and
 *          AHB, APB peripheral IPs
 * PCS3 --> Power domain for Core0 and L2C
 * PCSN --> Power domain for Core (N-3)
 */

#define PCS0_CFG_OFFSET             0x80
#define PCSm_CFG_OFFSET(i)          ((i + 3) * 0x20 + PCS0_CFG_OFFSET)
#define PCS_CFG_LIGHT_SLEEP_SHIFT   2
#define PCS_CFG_LIGHT_SLEEP         (1 << PCS_CFG_LIGHT_SLEEP_SHIFT)
#define PCS_CFG_DEEP_SLEEP_SHIFT    3
#define PCS_CFG_DEEP_SLEEP          (1 << PCS_CFG_DEEP_SLEEP_SHIFT)

#define PCS0_WE_OFFSET              0x90
#define PCSm_WE_OFFSET(i)           ((i + 3) * 0x20 + PCS0_WE_OFFSET)
// wakeup events source offset
#define PCS_WAKE_DBG_OFFSET         28
#define PCS_WAKE_MSIP_OFFSET        29

#define PCS0_CTL_OFFSET             0x94
#define PCSm_CTL_OFFSET(i)          ((i + 3) * 0x20 + PCS0_CTL_OFFSET)
#define PCS_CTL_CMD_SHIFT           0
#define PCS_CTL_PARAM_SHIFT         3
#define SLEEP_CMD                   0x3
#define WAKEUP_CMD                  (0x0 | (1 << PCS_CTL_PARAM_SHIFT))
#define LIGHT_SLEEP_MODE            0
#define DEEP_SLEEP_MODE             1
#define LIGHT_SLEEP_CMD             (SLEEP_CMD | (LIGHTSLEEP_MODE << PCS_CTL_PARAM_SHIFT))
#define DEEP_SLEEP_CMD              (SLEEP_CMD | (DEEPSLEEP_MODE << PCS_CTL_PARAM_SHIFT))

#define PCS0_STATUS_OFFSET          0x98
#define PCSm_STATUS_OFFSET(i)       ((i + 3) * 0x20 + PCS0_STATUS_OFFSET)
// PD* mask
#define PD_TYPE_MASK                0x7
#define PD_STATUS_MASK              0xf8
#define GET_PD_TYPE(val)            (val & PD_TYPE_MASK)
#define GET_PD_STATUS(val)          ((val & PD_STATUS_MASK) >> 3)
// PCS_STATUS[0:2]: pd_type
#define ACTIVE                      0
#define RESET                       1
#define SLEEP                       2
// PCS_STATUS[7:3]: pd_status for sleep mode
#define LIGHT_SLEEP_STATUS          0
#define DEEP_SLEEP_STATUS           16

#define RESET_VEC_LO_OFFSET         0x50
#define RESET_VEC_HI_OFFSET         0x60
#define RESET_VEC_8CORE_OFFSET      0x1a0
#define HARTn_RESET_VEC_LO(n)       \
    (RESET_VEC_LO_OFFSET +          \
    ((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + ((n) * 0x4))
#define HARTn_RESET_VEC_HI(n)       \
    (RESET_VEC_HI_OFFSET +          \
    ((n) < 4 ? 0 : RESET_VEC_8CORE_OFFSET) + ((n) * 0x4))
#define PCS_MAX_NR                  8
#define FLASH_BASE                  0x80000000ULL

#ifndef __ASSEMBLER__

#include <sbi/sbi_types.h>

struct smu_data {
	unsigned long addr;
};

extern struct smu_data smu;
extern u32 ae350_suspend_mode[];

int smu_set_wakeup_events(struct smu_data *smu, u32 events, u32 hartid);
bool smu_support_sleep_mode(struct smu_data *smu, u32 sleep_mode, u32 hartid);
int smu_set_command(struct smu_data *smu, u32 pcs_ctl, u32 hartid);
int smu_set_reset_vector(struct smu_data *smu, ulong wakeup_addr, u32 hartid);

void smu_suspend_prepare(int main_core, bool enable);
void smu_set_sleep(int cpu, unsigned char sleep);
void smu_set_wakeup_enable(int cpu, bool main_core, unsigned int events);
void smu_check_pcs_status(int sleep_mode_status, int num_cpus);
int get_pd_type(unsigned int cpu);
int get_pd_status(unsigned int cpu);

#endif /* __ASSEMBLER__  */

#endif /* _SYS_ATCSMU_H */
