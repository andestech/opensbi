/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *   Nylon Chen <nylon7@andestech.com>
 */

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
		csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_WB_ALL);

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
