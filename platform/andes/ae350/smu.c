/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *   Dylan <dylan@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include "smu.h"

int ae350_suspend_mode=0;
void smu_suspend_prepare(char main_core, char enable){
	if (main_core) {
		if (enable) {
			csr_set(CSR_MIE, MIP_MTIP);
			csr_set(CSR_MIE, MIP_MSIP);
			csr_set(CSR_MIE, MIP_MEIP);
		} else {
			csr_clear(CSR_MIE, MIP_MTIP);
			csr_clear(CSR_MIE, MIP_MSIP);
			csr_clear(CSR_MIE, MIP_MEIP);
		}

	} else {
		if (enable) {
			csr_set(CSR_MIE, MIP_MEIP);
			csr_set(CSR_MIE, MIP_MSIP);
			csr_set(CSR_MIE, MIP_MTIP);
		} else {
			csr_clear(CSR_MIE, MIP_MEIP);
			csr_clear(CSR_MIE, MIP_MTIP);

			// enable IPI
			csr_set(CSR_MIE, MIP_MSIP);
		}
	}
}

void smu_set_sleep(int cpu, unsigned char sleep)
{
	volatile void *smu_pcs_ctl_base = (void *)((unsigned long)SMU_BASE + PCSm_CTL_OFF(cpu));
	unsigned long smu_val = readl(smu_pcs_ctl_base);
	unsigned char *ctl = (unsigned char *)&smu_val;

	// set sleep cmd
	*ctl = 0;
	*ctl = *ctl | SLEEP_CMD;
	// set param
	*ctl = *ctl | (sleep << PCS_CTL_PARAM_OFF);
	writel(smu_val, smu_pcs_ctl_base);
}

void smu_set_wakeup_enable(int cpu, unsigned int events)
{
	volatile void *smu_we_base = (void *)((unsigned long)SMU_BASE + PCSm_WE_OFF(cpu));
	if (cpu == 0)
		events |= (1 << PCS_WAKE_DBG_OFF);

	writel(events, smu_we_base);
}