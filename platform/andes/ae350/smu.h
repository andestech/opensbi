// See LICENSE for license details.

#ifndef _RISCV_SMU_H
#define _RISCV_SMU_H


#if __riscv_xlen == 64
# define SLL32    sllw
# define STORE    sd
# define LOAD     ld
# define LWU      lwu
# define LOG_REGBYTES 3
#else
# define SLL32    sll
# define STORE    sw
# define LOAD     lw
# define LWU      lw
# define LOG_REGBYTES 2
#endif
#define REGBYTES (1 << LOG_REGBYTES)
#define PUSH(which) addi sp, sp, -REGBYTES; STORE which, 0(sp)
#define PUSH_CSR(csr) addi sp, sp, -REGBYTES; csrr t0, csr; STORE t0, 0(sp)
#define POP(which)  LOAD which, 0(sp); addi sp, sp, REGBYTES
#define POP_CSR(csr)  LOAD t0, 0(sp); addi sp, sp, REGBYTES; csrw csr, t0

#define FLASH_BASE		0x80000000
#define SMU_BASE		0xf0100000 // [TODO] get from fdt
#define SMUCR_OFF		0x14
#define SMUCR_RESET		0x3c
#define SMU_RESET_VEC_LO_OFF		0x50
#define SMU_RESET_VEC_HI_OFF		0x60
#define SMU_HARTn_RESET_VEC_LO(n) (SMU_RESET_VEC_LO_OFF + (n * 0x4))
#define SMU_HARTn_RESET_VEC_HI(n) (SMU_RESET_VEC_HI_OFF + (n * 0x4))
#define PCS_RESET           0x1

#define MAX_PCS_SLOT    7

#define PCS0_WE_OFF     0x90
#define PCS0_CTL_OFF    0x94
#define PCS0_STATUS_OFF 0x98

/*
 * PCS0 --> Always on power domain, includes the JTAG tap and DMI_AHB bus in
 *  ncejdtm200.
 * PCS1 --> Power domain for debug subsystem
 * PCS2 --> Main power domain, includes the system bus and AHB, APB peripheral
 *  IPs.
 * PCS3 --> Power domain for Core0 and L2C.
 * PCSN --> Power domain for Core (N-3)
 */
#define PCSm_WE_OFF(i)        ((i + 3) * 0x20 + PCS0_WE_OFF)
#define PCSm_STATUS_OFF(i)    ((i + 3) * 0x20 + PCS0_STATUS_OFF)
#define PCSm_CTL_OFF(i)       ((i + 3) * 0x20 + PCS0_CTL_OFF)

// PD* mask
#define PD_TYPE_MASK        0x7
#define PD_STATUS_MASK      0xf8
#define GET_PD_TYPE(val)    (val & PD_TYPE_MASK)
#define GET_PD_STATUS(val)  ((val & PD_STATUS_MASK) >> 3)

// PCS_STATUS[0:2]: pd_type
#define ACTIVE  0
#define RESET   1
#define SLEEP   2

// PCS_STATUS[7:3]: pd_status for sleep mode
#define LightSleep_STATUS       0
#define DeepSleep_STATUS        16

// wakeup events source offset
#define PCS_WAKE_DBG_OFF	28
#define PCS_WAKE_MSIP_OFF	29

// PCS_CTL
#define PCS_CTL_PARAM_OFF       3
#define SLEEP_CMD               3

// param of PCS_CTL for sleep cmd
#define LightSleep_CTL          0
#define DeepSleep_CTL           1

#define NormalMode              0
#define LightSleepMode          1
#define DeepSleepMode           2
#define CpuHotplugDeepSleepMode 3


#ifndef __ASSEMBLER__

struct smu_data {
	unsigned long addr;
};

void smu_suspend_prepare(char main_core, char enable);
void smu_set_sleep(int cpu, unsigned char sleep);
void smu_set_wakeup_enable(int cpu, int main_core, unsigned int events);
int get_pd_type(unsigned int cpu);
int get_pd_status(unsigned int cpu);
void smu_check_pcs_status(int sleep_mode_status, int num_cpus);
#endif /* __ASSEMBLY__ */

#endif /* _RISCV_SMU_H */
