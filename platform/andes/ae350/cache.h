/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *   Nylon Chen <nylon7@andestech.com>
 */

#include "platform.h"

#define MAX_CACHE_LINE_SIZE 256

/* L2 cache registers */
#define L2C_REG_CFG_OFFSET	0
#define L2C_REG_CTL_OFFSET	0x8
#define L2C_HPM_C0_CTL_OFFSET	0x10
#define L2C_HPM_C1_CTL_OFFSET	0x18
#define L2C_HPM_C2_CTL_OFFSET	0x20
#define L2C_HPM_C3_CTL_OFFSET	0x28
#define L2C_REG_C0_CMD_OFFSET	0x40
#define L2C_REG_C0_ACC_OFFSET	0x48
#define L2C_REG_C1_CMD_OFFSET	0x50
#define L2C_REG_C1_ACC_OFFSET	0x58
#define L2C_REG_C2_CMD_OFFSET	0x60
#define L2C_REG_C2_ACC_OFFSET	0x68
#define L2C_REG_C3_CMD_OFFSET	0x70
#define L2C_REG_C3_ACC_OFFSET	0x78
#define L2C_REG_STATUS_OFFSET	0x80
#define L2C_REG_C0_HPM_OFFSET	0x200

/* L2 CCTL status */
#define CCTL_L2_STATUS_IDLE	0
#define CCTL_L2_STATUS_PROCESS	1
#define CCTL_L2_STATUS_ILLEGAL	2
/* L2 CCTL status cores mask */
#define CCTL_L2_STATUS_C0_MASK	0xF
#define CCTL_L2_STATUS_C1_MASK	0xF0
#define CCTL_L2_STATUS_C2_MASK	0xF00
#define CCTL_L2_STATUS_C3_MASK	0xF000

/* L2 cache operation */
#define CCTL_L2_PA_INVAL	0x8
#define CCTL_L2_PA_WB		0x9
#define CCTL_L2_PA_WBINVAL	0xA
#define CCTL_L2_WBINVAL_ALL	0x12

#define L2C_HPM_PER_CORE_OFFSET		0x8
#define L2C_REG_PER_CORE_OFFSET		0x10
#define CCTL_L2_STATUS_PER_CORE_OFFSET	4
#define L2C_REG_CN_CMD_OFFSET(n)	\
	L2C_REG_C0_CMD_OFFSET + (n * L2C_REG_PER_CORE_OFFSET)
#define L2C_REG_CN_ACC_OFFSET(n)	\
	L2C_REG_C0_ACC_OFFSET + (n * L2C_REG_PER_CORE_OFFSET)
#define CCTL_L2_STATUS_CN_MASK(n)	\
	CCTL_L2_STATUS_C0_MASK << (n * CCTL_L2_STATUS_PER_CORE_OFFSET)
#define L2C_HPM_CN_CTL_OFFSET(n)	\
	L2C_HPM_C0_CTL_OFFSET + (n * L2C_HPM_PER_CORE_OFFSET)
#define L2C_REG_CN_HPM_OFFSET(n)	\
	L2C_REG_C0_HPM_OFFSET + (n * L2C_HPM_PER_CORE_OFFSET)

void cpu_dcache_inval_line(unsigned long start, unsigned long last_hartid);
void cpu_dcache_wb_line(unsigned long start, unsigned long last_hartid);
void cpu_dcache_wbinval_all(unsigned long start, unsigned long last_hartid);
void cpu_dma_inval_range(unsigned long pa, unsigned long size, unsigned long last_hartid);
void cpu_dma_wb_range(unsigned long pa, unsigned long size, unsigned long last_hartid);

uintptr_t mcall_set_mcache_ctl(unsigned long input);
uintptr_t mcall_set_mmisc_ctl(unsigned long input);
uintptr_t mcall_icache_op(unsigned int enable);
uintptr_t mcall_dcache_wbinval_all(void);
uintptr_t mcall_l1_cache_i_prefetch_op(unsigned long enable);
uintptr_t mcall_l1_cache_d_prefetch_op(unsigned long enable);
uintptr_t mcall_non_blocking_load_store(unsigned long enable);
uintptr_t mcall_write_around(unsigned long enable);

/*
 * In 45-series, When D-cache is disabled, CM also needs to be disabled together.
 * When D-cache is disabled, all the load/store will directly access to the memory(not L2).
 *
 * Scenario:
 *   mcall_dcache_op(0);
 *   mcall_dcache_op(1);   -->  prologue: addi    sp,sp,-16
 *                                        sw      ra,12(sp)    <-- ra stores to "Memory"
 *                                        .....
 *                              Enable D-cache & CM
 *                                        .....
 *                              Epilogue: c.lwsp  ra,12(sp)    <-- read sp from "L2"
 *                                        c.addi  sp,16            & store back to ra
 *                                        ret
 * ---------------------------------------------------------------------------------------
 * We can see that the function prologue & epilogue store & restore the "sp" register
 * in different memory region, which will cause load access fault later.
 *
 * There are 2 way to avoid this situation:
 *    1. invalid "sp" addr in both L1 & L2, make sure "sp" get the newest value from memory.
 *    2. Declare mcall_dcache_op() as inline or macro to eliminate prologue & epilogue.
 */
static inline __attribute__((always_inline)) uintptr_t mcall_dcache_op(unsigned int enable)
{
	if (enable) {
		if (is_andestar45_series()) {
			uintptr_t mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
			if (!(mcache_ctl_val & V5_MCACHE_CTL_DC_COHEN_EN))
				mcache_ctl_val |= V5_MCACHE_CTL_DC_COHEN_EN;

			csr_write(CSR_MCACHE_CTL, mcache_ctl_val);

			/*
			 * Check DC_COHEN_EN, if cannot write to mcache_ctl,
			 * we assume this bitmap not support L2 CM
			 */
			mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
			if ((mcache_ctl_val & V5_MCACHE_CTL_DC_COHEN_EN)) {
				/* Wait for DC_COHSTA bit be set */
				while (!(mcache_ctl_val & V5_MCACHE_CTL_DC_COHSTA_EN))
					mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
			}
		}

		csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_INVAL_ALL);
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_DC_EN);
	} else {
		csr_clear(CSR_MCACHE_CTL, V5_MCACHE_CTL_DC_EN);
		csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_WBINVAL_ALL);

		if (is_andestar45_series()) {
			uintptr_t mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
			if ((mcache_ctl_val & V5_MCACHE_CTL_DC_COHEN_EN))
				mcache_ctl_val &= ~V5_MCACHE_CTL_DC_COHEN_EN;

			csr_write(CSR_MCACHE_CTL, mcache_ctl_val);

			mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
			if (!(mcache_ctl_val & V5_MCACHE_CTL_DC_COHEN_EN)) {
				/* Wait for DC_COHSTA bit be clear */
				while ((mcache_ctl_val & V5_MCACHE_CTL_DC_COHSTA_EN))
					mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
			}
		}
	}
	return 0;
}
