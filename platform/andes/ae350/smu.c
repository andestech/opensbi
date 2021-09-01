/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *   Dylan <dylan@andestech.com>
 */

#include <sbi/sbi_types.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/riscv_io.h>
#include "smu.h"
#include "platform.h"

int ae350_suspend_mode[AE350_HART_COUNT] = {0};
void smu_suspend_prepare(char main_core, char enable)
{
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
	volatile void *smu_pcs_ctl_base = (void *)((unsigned long)SMU_BASE + CN_PCS_CTL_OFF(cpu));
	unsigned long smu_val = readl(smu_pcs_ctl_base);
	unsigned char *ctl = (unsigned char *)&smu_val;

	// set sleep cmd
	*ctl = 0;
	*ctl = *ctl | SLEEP_CMD;
	// set param
	*ctl = *ctl | (sleep << PCS_CTL_PARAM_OFF);
	writel(smu_val, smu_pcs_ctl_base);
}

void smu_set_wakeup_enable(int cpu, int main_core, unsigned int events)
{
	volatile void *smu_we_base = (void *)((unsigned long)SMU_BASE + CN_PCS_WE_OFF(cpu));
	if (main_core)
		events |= (1 << PCS_WAKE_DBG_OFF);

	writel(events, smu_we_base);
}

int get_pd_type(unsigned int cpu)
{
	volatile void *smu_pcs_status_base =
		(void *)((unsigned long)SMU_BASE + CN_PCS_STATUS_OFF(cpu));
	unsigned long smu_pcs_status_val = readl(smu_pcs_status_base);

	return GET_PD_TYPE(smu_pcs_status_val);
}

int get_pd_status(unsigned int cpu)
{
	volatile void *smu_pcs_status_base =
		(void *)((unsigned long)SMU_BASE + CN_PCS_STATUS_OFF(cpu));
		unsigned long smu_pcs_status_val = readl(smu_pcs_status_base);

	return GET_PD_STATUS(smu_pcs_status_val);
}

void smu_check_pcs_status(int sleep_mode_status, int num_cpus)
{
	int ready_cnt = num_cpus - 1;
	int ready_cpu[AE350_HART_COUNT] = {0};
	u32 hartid = current_hartid();
	u32 cpu, status, type;

	while (ready_cnt) {
		for (cpu = 0; cpu < num_cpus; cpu++) {
			if (cpu == hartid || ready_cpu[cpu] == 1)
				continue;

			type = get_pd_type(cpu);
			status = get_pd_status(cpu);

			if (type == SLEEP && status == sleep_mode_status) {
				ready_cnt--;
				ready_cpu[cpu] = 1;
			}
		}
	}
}

