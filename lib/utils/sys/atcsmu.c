/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi_utils/sys/atcsmu.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_bitops.h>
#include <sbi/riscv_asm.h>
#include <andes/andes_sbi.h>
#include <andes/andesv5.h>

u32 ae350_suspend_mode[AE350_HART_COUNT_MAX] = { 0 };
uintptr_t MIE_BACKUP[AE350_HART_COUNT_MAX]   = { 0 };
uintptr_t SIE_BACKUP[AE350_HART_COUNT_MAX]   = { 0 };

int get_pd_type(unsigned int cpu)
{
	volatile void *smu_pcs_status_base =
		(void *)(smu.addr + PCSm_STATUS_OFFSET(cpu));
	unsigned long smu_pcs_status_val = readl(smu_pcs_status_base);

	return GET_PD_TYPE(smu_pcs_status_val);
}

int get_pd_status(unsigned int cpu)
{
	volatile void *smu_pcs_status_base =
		(void *)(smu.addr + PCSm_STATUS_OFFSET(cpu));
	unsigned long smu_pcs_status_val = readl(smu_pcs_status_base);

	return GET_PD_STATUS(smu_pcs_status_val);
}

void smu_set_wakeup_enable(int cpu, bool main_core, unsigned int events)
{
	volatile void *smu_we_base = (void *)(smu.addr + PCSm_WE_OFFSET(cpu));
	if (main_core)
		events |= (1 << PCS_WAKE_DBG_OFFSET);

	writel(events, smu_we_base);
}

void smu_suspend_prepare(int main_core, bool enable)
{
	u32 hartid = current_hartid();

	if (enable) {
		csr_write(CSR_SIE, SIE_BACKUP[hartid]);
		csr_write(CSR_MIE, MIE_BACKUP[hartid]);
	} else {
		SIE_BACKUP[hartid] = csr_read(CSR_SIE);
		MIE_BACKUP[hartid] = csr_read(CSR_MIE);

		csr_write(CSR_SIE, 0);
		csr_write(CSR_MIE, 0);

		if (main_core == 1) {
			/*
			 * Enable external interrupt.
			 * For Andes, we enabled SEIP because our peripherals
			 * are registered to S-mode context.
			 */
			csr_set(CSR_SIE, MIP_SEIP);
		} else if (main_core == 0) {
			/* Other cores enable IPI */
			csr_set(CSR_MIE, MIP_MSIP);
		}
	}
}

void smu_set_sleep(int cpu, unsigned char sleep)
{
	volatile void *smu_pcs_ctl_base =
		(void *)(smu.addr + PCSm_CTL_OFFSET(cpu));
	unsigned long smu_val = readl(smu_pcs_ctl_base);
	unsigned char *ctl    = (unsigned char *)&smu_val;

	// set cmd & param
	*ctl = SLEEP_CMD | (sleep << PCS_CTL_PARAM_SHIFT);
	writel(smu_val, smu_pcs_ctl_base);
}

void smu_check_pcs_status(int sleep_mode_status, int num_cpus)
{
	int active_cnt			    = num_cpus - 1;
	int ready_cpu[AE350_HART_COUNT_MAX] = { 0 };
	u32 hartid			    = current_hartid();
	u32 cpu, status, type;

	while (active_cnt) {
		for (cpu = 0; cpu < num_cpus; cpu++) {
			if (cpu == hartid || ready_cpu[cpu] == 1)
				continue;

			type   = get_pd_type(cpu);
			status = get_pd_status(cpu);

			if (type == SLEEP && status == sleep_mode_status) {
				active_cnt--;
				ready_cpu[cpu] = 1;
			}
		}
	}
}

inline int smu_set_wakeup_events(struct smu_data *smu, u32 events, u32 hartid)
{
	if (smu) {
		writel(events, (void *)(smu->addr + PCSm_WE_OFFSET(hartid)));
		return 0;
	} else
		return SBI_EINVAL;
}

inline bool smu_support_sleep_mode(struct smu_data *smu, u32 sleep_mode,
				   u32 hartid)
{
	u32 pcs_cfg;

	if (!smu) {
		sbi_printf("%s(): Failed to access smu_data\n", __func__);
		return false;
	}

	pcs_cfg = readl((void *)(smu->addr + PCSm_CFG_OFFSET(hartid)));

	switch (sleep_mode) {
	case LIGHT_SLEEP_MODE:
		if (EXTRACT_FIELD(pcs_cfg, PCS_CFG_LIGHT_SLEEP) == 0) {
			sbi_printf(
				"SMU: hart%d (PCS%d) does not support light sleep mode\n",
				hartid, hartid + 3);
			return false;
		}
		break;
	case DEEP_SLEEP_MODE:
		if (EXTRACT_FIELD(pcs_cfg, PCS_CFG_DEEP_SLEEP) == 0) {
			sbi_printf(
				"SMU: hart%d (PCS%d) does not support deep sleep mode\n",
				hartid, hartid + 3);
			return false;
		}
		break;
	}

	return true;
}

inline int smu_set_command(struct smu_data *smu, u32 pcs_ctl, u32 hartid)
{
	if (smu) {
		writel(pcs_ctl, (void *)(smu->addr + PCSm_CTL_OFFSET(hartid)));
		return 0;
	} else
		return SBI_EINVAL;
}

inline int smu_set_reset_vector(struct smu_data *smu, ulong wakeup_addr,
				u32 hartid)
{
	u32 vec_lo, vec_hi;
	u64 reset_vector;

	if (!smu)
		return SBI_EINVAL;

	writel(wakeup_addr, (void *)(smu->addr + HARTn_RESET_VEC_LO(hartid)));
	writel((u64)wakeup_addr >> 32,
	       (void *)(smu->addr + HARTn_RESET_VEC_HI(hartid)));

	vec_lo = readl((void *)(smu->addr + HARTn_RESET_VEC_LO(hartid)));
	vec_hi = readl((void *)(smu->addr + HARTn_RESET_VEC_HI(hartid)));
	reset_vector = ((u64)vec_hi << 32) | vec_lo;

	if (reset_vector != (u64)wakeup_addr) {
		sbi_printf(
			"hard%d (PCS%d): Failed to program the reset vector.\n",
			hartid, hartid + 3);
		return SBI_EFAIL;
	} else
		return 0;
}
