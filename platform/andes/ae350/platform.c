/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 *   Nylon Chen <nylon7@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include "platform.h"
#include "plicsw.h"
#include "plmt.h"
#include "cache.h"
#include "trigger.h"
#include "smu.h"
#include "pma.h"

static struct plic_data plic = {
	.addr = AE350_PLIC_ADDR,
	.num_src = AE350_PLIC_NUM_SOURCES,
};
int has_l2;

static int ae350_pre_init(bool cold_boot)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	/* enable L1 cache */
	uintptr_t mcache_ctl_val = csr_read(CSR_MCACHE_CTL);
	uintptr_t mmisc_ctl_val = csr_read(CSR_MMISC_CTL);

	if (is_andestar45_series()) {
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

	if (!(mcache_ctl_val & V5_MCACHE_CTL_IC_EN))
		mcache_ctl_val |= V5_MCACHE_CTL_IC_EN;
	if ((!(scratch->options & SBI_SCRATCH_NO_CACHE_PRINTS)) || (!(cold_boot))) {
		if (!(mcache_ctl_val & V5_MCACHE_CTL_DC_EN))
			mcache_ctl_val |= V5_MCACHE_CTL_DC_EN;
	}
	if (!(mcache_ctl_val & V5_MCACHE_CTL_CCTL_SUEN))
		mcache_ctl_val |= V5_MCACHE_CTL_CCTL_SUEN;
	if (!(mcache_ctl_val & V5_MCACHE_CTL_L1I_PREFETCH_EN))
		mcache_ctl_val |= V5_MCACHE_CTL_L1I_PREFETCH_EN;
	if (!(mcache_ctl_val & V5_MCACHE_CTL_L1D_PREFETCH_EN))
		mcache_ctl_val |= V5_MCACHE_CTL_L1D_PREFETCH_EN;
	if (!(mcache_ctl_val & V5_MCACHE_CTL_DC_WAROUND_1_EN))
		mcache_ctl_val |= V5_MCACHE_CTL_DC_WAROUND_1_EN;
	if (!(mcache_ctl_val & V5_MCACHE_CTL_L2C_WAROUND_1_EN))
		mcache_ctl_val |= V5_MCACHE_CTL_L2C_WAROUND_1_EN;

	csr_write(CSR_MCACHE_CTL, mcache_ctl_val);

	/* enable non-blocking load */
	if (!(mmisc_ctl_val & V5_MMISC_CTL_NON_BLOCKING_EN))
		mmisc_ctl_val |= V5_MMISC_CTL_NON_BLOCKING_EN;

	csr_write(CSR_MMISC_CTL, mmisc_ctl_val);

	/* enable L2 cache */
	uint32_t *l2c_cfg_base = (void*)AE350_L2C_ADDR;
	uint32_t *l2c_ctl_base = (void *)AE350_L2C_ADDR + V5_L2C_CTL_OFFSET;
	uint32_t l2c_cfg_val = *l2c_cfg_base;
	uint32_t l2c_ctl_val = *l2c_ctl_base;
	uint32_t version = (l2c_cfg_val & V5_L2C_CFG_VERSION_MASK) >> V5_L2C_CFG_VERSION_OFFSET;

	/* l2c_ctl_val=0xffffffff  ==> no_l2 */
	has_l2 = (l2c_ctl_val == ((uint32_t)~0U)) ? 0 : 1;
	if (has_l2) {
		if (!(l2c_ctl_val & V5_L2C_CTL_ENABLE_MASK))
			l2c_ctl_val |= V5_L2C_CTL_ENABLE_MASK |
						V5_L2C_CTL_IPFDPT_MASK |
						V5_L2C_CTL_DPFDPT_MASK;

		/* Gen2 L2 defaults to on, so needs extra setting */
		if (version >= 16)
			l2c_ctl_val |= V5_L2C_CTL_IPFDPT_MASK | V5_L2C_CTL_DPFDPT_MASK;

		/* ram cycle settings */
		/*
		 * **Do NOT** change RAM cycle settings for Andes' EVB, but once you
		 * you have done the integration, please uncomment the following and
		 * do the proper setting for your platform.
		 */
		// l2c_ctl_val |= scan->tag_ram_ctl[0] << V5_L2C_CTL_TRAMOCTL_OFFSET;
		// l2c_ctl_val |= scan->tag_ram_ctl[1] << V5_L2C_CTL_TRAMICTL_OFFSET;
		// l2c_ctl_val |= scan->data_ram_ctl[0] << V5_L2C_CTL_DRAMOCTL_OFFSET;
		// l2c_ctl_val |= scan->data_ram_ctl[1] << V5_L2C_CTL_DRAMICTL_OFFSET;

		*l2c_ctl_base = l2c_ctl_val;
	}

	return 0;
}

/* Platform early initialization. */
static int ae350_early_init(bool cold_boot)
{
	/* Machine counter write enable */
	csr_write(CSR_MCOUNTERWEN, 0xfffffffd);
	/* Supervisor local interrupt enable */
	csr_write(CSR_SLIE, MIP_MOVFIP);
	/* disable machine counter in M-mode */
	csr_write(CSR_MCOUNTERMASK_M, 0xfffffffd);
	/* delegate S-mode local interrupt to S-mode */
	csr_write(CSR_MSLIDELEG, MIP_MOVFIP);

	return 0;
}

/* Platform final initialization. */
static int ae350_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	fdt_fixups(fdt);

	init_pma();
	trigger_init();

	return 0;
}

static uintptr_t mcall_set_trigger(long type, uintptr_t data, unsigned int m,
					unsigned int s, unsigned int u)
{
	int ret;
	switch (type) {
	case TRIGGER_TYPE_ICOUNT:
		ret = trigger_set_icount(data, m, s, u);
		break;
	case TRIGGER_TYPE_ITRIGGER:
		ret = trigger_set_itrigger(data, m, s, u);
		break;
	case TRIGGER_TYPE_ETRIGGER:
		ret = trigger_set_etrigger(data, m, s, u);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static uintptr_t mcall_set_pfm(void)
{
	csr_clear(CSR_SLIP, MIP_SOVFIP);
	csr_set(CSR_MIE, MIP_MOVFIP);
	return 0;
}

static uintptr_t mcall_suspend_prepare(char main_core, char enable)
{
	smu_suspend_prepare(main_core, enable);
	return 0;
}

extern void cpu_suspend2ram(void);
static uintptr_t mcall_suspend_backup(void)
{
	cpu_suspend2ram();
	return 0;
}

static void mcall_restart(unsigned int cpu_num)
{
	int i;
	unsigned int *dev_ptr;			/* smu reset vector register is 32 bit */
	unsigned char *cmd;				/* smu reset cmd register is 8 bit */


	for (i = 0; i < cpu_num; i++) {
		dev_ptr = (unsigned int *)((unsigned long)SMU_BASE + SMU_RESET_VEC_OFF
			+ SMU_RESET_VEC_PER_CORE * i);
		*dev_ptr = DRAM_BASE;
	}

	dev_ptr = (unsigned int *)((unsigned long)SMU_BASE + SMUCR_OFF);
	cmd = (unsigned char *)dev_ptr;
	*cmd = SMUCR_RESET;

	asm volatile("ebreak");			/* should not enter here */
}

extern void cpu_resume(void);
static void mcall_set_reset_vec(int cpu_nums)
{
	int i;
	unsigned int *dev_ptr;
	unsigned int *tmp = (unsigned int *)&cpu_resume;

	for (i = 0; i < cpu_nums; i++) {
		dev_ptr = (unsigned int *)((unsigned long)SMU_BASE + SMU_RESET_VEC_OFF
			+ SMU_RESET_VEC_PER_CORE*i);

	*dev_ptr = (unsigned long)tmp;
	}
}

/* Initialize the platform console. */
static int ae350_console_init(void)
{
	return uart8250_init(AE350_UART_ADDR,
				 AE350_UART_FREQUENCY,
				 AE350_UART_BAUDRATE,
				 AE350_UART_REG_SHIFT,
				 AE350_UART_REG_WIDTH);
}

/* Initialize the platform interrupt controller for current HART. */
static int ae350_irqchip_init(bool cold_boot)
{
	u32 hartid = current_hartid();
	int ret;

	if (cold_boot) {
		ret = plic_cold_irqchip_init(&plic);
		if (ret)
			return ret;
	}

	return plic_warm_irqchip_init(&plic, 2 * hartid, 2 * hartid + 1);
}

/* Initialize IPI for current HART. */
static int ae350_ipi_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = plicsw_cold_ipi_init(AE350_PLICSW_ADDR,
					   AE350_HART_COUNT);
		if (ret)
			return ret;
	}

	return plicsw_warm_ipi_init();
}

/* Initialize platform timer for current HART. */
static int ae350_timer_init(bool cold_boot)
{
	int ret;

	if (cold_boot) {
		ret = plmt_cold_timer_init(AE350_PLMT_ADDR,
					   AE350_HART_COUNT);
		if (ret)
			return ret;
	}

	return plmt_warm_timer_init();
}

/* Reset the platform. */
static int ae350_system_reset(u32 type)
{
	/* For now nothing to do. */
	sbi_printf("System reset\n");
	return 0;
}

/* called flow:
 *
 *	1. kernel -> ae350_set_suspend_mode(light/deep) -> set variable: ae350_suspend_mode
 *
 *	2. cpu_stop() -> sbi_hsm_hart_stop() -> sbi_hsm_exit() ->
 *		jump_warmboot() -> sbi_hsm_hart_wait() -> ae350_enter_suspend_mode() -> normal/light/deep
 */
int ae350_set_suspend_mode(int suspend_mode)
{
	u32 hartid = current_hartid();

	ae350_suspend_mode[hartid] = suspend_mode;
	return 0;
}

int ae350_enter_suspend_mode(int suspend_mode, int main_core,
				unsigned int wake_mask, int num_cpus)
{
	u32 hartid = current_hartid();

	// smu function
	if (suspend_mode == LightSleepMode) {
		sbi_printf("%s(): CPU[%d] LightSleepMode\n", __func__, hartid);

		// set SMU wakeup enable & MISC control
		smu_set_wakeup_enable(hartid, main_core, wake_mask);
		// Disable higher privilege's non-wakeup event
		smu_suspend_prepare(main_core, false);
		// set SMU light sleep command
		smu_set_sleep(hartid, LightSleep_CTL);
		// Wait for other cores to enter sleeping mode
		if (main_core)
			smu_check_pcs_status(LightSleep_STATUS, num_cpus);
		// D-cache disable
		mcall_dcache_op(0);
		// wait for interrupt
		wfi();
		// D-cache enable
		mcall_dcache_op(1);
		// enable privilege
		smu_suspend_prepare(main_core, true);
	} else if (suspend_mode == DeepSleepMode) {
		sbi_printf("%s(): CPU[%d] DeepSleepMode\n", __func__, hartid);

		// set SMU wakeup enable & MISC control
		smu_set_wakeup_enable(hartid, main_core, wake_mask);
		// Disable higher privilege's non-wakeup event
		smu_suspend_prepare(main_core, false);
		// set SMU Deep sleep command
		smu_set_sleep(hartid, DeepSleep_CTL);
		// Wait for other cores to enter sleeping mode
		if (main_core)
			smu_check_pcs_status(DeepSleep_STATUS, num_cpus);
		// stop & wfi & resume
		cpu_suspend2ram();
		// enable privilege
		smu_suspend_prepare(main_core, true);
	} else if (suspend_mode == CpuHotplugDeepSleepMode) {
		/*
		 * In 25-series, core 0 is binding with L2 power domain,
		 * core 0 should NOT enter deep sleep mode.
		 *
		 * For 45-series, every core has its own power domain,
		 * It's ok to sleep main core.
		 */
		if (is_andestar45_series() || !(hartid == 0)) {
			sbi_printf("%s(): CPU[%d] Cpu Hotplug DeepSleepMode\n",
				__func__, hartid);

			// set SMU wakeup enable & MISC control
			smu_set_wakeup_enable(hartid, main_core, 0);
			// Disable higher privilege's non-wakeup event
			smu_suspend_prepare(-1, false);
			// set SMU Deep sleep command
			smu_set_sleep(hartid, DeepSleep_CTL);
			// stop & wfi & resume
			cpu_suspend2ram();
			// enable privilege
			smu_suspend_prepare(-1, true);
		}
	}

	return 0;
}

/* Vendor-Specific SBI handler */
static int ae350_vendor_ext_provider(long extid, long funcid,
	unsigned long *args, unsigned long *out_value,
	struct sbi_trap_info *out_trap)
{
	int ret = 0;
	switch (funcid) {
	case SBI_EXT_ANDES_GET_MCACHE_CTL_STATUS:
		*out_value = csr_read(CSR_MCACHE_CTL);
		break;
	case SBI_EXT_ANDES_GET_MMISC_CTL_STATUS:
		*out_value = csr_read(CSR_MMISC_CTL);
		break;
	case SBI_EXT_ANDES_SET_MCACHE_CTL:
		ret = mcall_set_mcache_ctl(args[0]);
		break;
	case SBI_EXT_ANDES_SET_MMISC_CTL:
		ret = mcall_set_mmisc_ctl(args[0]);
		break;
	case SBI_EXT_ANDES_ICACHE_OP:
		ret = mcall_icache_op(args[0]);
		break;
	case SBI_EXT_ANDES_DCACHE_OP:
		ret = mcall_dcache_op(args[0]);
		break;
	case SBI_EXT_ANDES_L1CACHE_I_PREFETCH:
		ret = mcall_l1_cache_i_prefetch_op(args[0]);
		break;
	case SBI_EXT_ANDES_L1CACHE_D_PREFETCH:
		ret = mcall_l1_cache_d_prefetch_op(args[0]);
		break;
	case SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE:
		ret = mcall_non_blocking_load_store(args[0]);
		break;
	case SBI_EXT_ANDES_WRITE_AROUND:
		ret = mcall_write_around(args[0]);
		break;
	case SBI_EXT_ANDES_TRIGGER:
		*out_value = mcall_set_trigger(args[0], args[1], 0, 0, args[2]);
		break;
	case SBI_EXT_ANDES_SET_PFM:
		ret = mcall_set_pfm();
		break;
	case SBI_EXT_ANDES_READ_POWERBRAKE:
		*out_value = csr_read(CSR_MPFT_CTL);
		break;
	case SBI_EXT_ANDES_WRITE_POWERBRAKE:
		csr_write(CSR_MPFT_CTL, args[0]);
		break;
	case SBI_EXT_ANDES_SUSPEND_PREPARE:
		ret = mcall_suspend_prepare(args[0], args[1]);
		break;
	case SBI_EXT_ANDES_SUSPEND_MEM:
		ret = mcall_suspend_backup();
		break;
	case SBI_EXT_ANDES_SET_SUSPEND_MODE:
		ae350_set_suspend_mode(args[0]);
		break;
	case SBI_EXT_ANDES_ENTER_SUSPEND_MODE:
		ae350_enter_suspend_mode(args[0], args[1], args[2], args[3]);
		break;
	case SBI_EXT_ANDES_RESTART:
		mcall_restart(args[0]);
		break;
	case SBI_EXT_ANDES_RESET_VEC:
		mcall_set_reset_vec(args[0]);
		break;
	case SBI_EXT_ANDES_SET_PMA:
		mcall_set_pma(args[0], args[1], args[2], args[3]);
		break;
	case SBI_EXT_ANDES_FREE_PMA:
		mcall_free_pma(args[0]);
		break;
	case SBI_EXT_ANDES_PROBE_PMA:
		*out_value = ((csr_read(CSR_MMSC_CFG) & 0x40000000) != 0);
		break;
	case SBI_EXT_ANDES_DCACHE_WBINVAL_ALL:
		ret = mcall_dcache_wbinval_all();
		break;
	default:
		sbi_printf("Unsupported vendor sbi call : %ld\n", funcid);
		asm volatile("ebreak");
	}
	return ret;
}

/* Platform descriptor. */
const struct sbi_platform_operations platform_ops = {
	.pre_init   = ae350_pre_init,
	.early_init = ae350_early_init,
	.final_init = ae350_final_init,

	.console_init = ae350_console_init,
	.console_putc = uart8250_putc,
	.console_getc = uart8250_getc,

	.irqchip_init = ae350_irqchip_init,

	.ipi_init     = ae350_ipi_init,
	.ipi_send     = plicsw_ipi_send,
	.ipi_clear    = plicsw_ipi_clear,

	.timer_init	   = ae350_timer_init,
	.timer_value	   = plmt_timer_value,
	.timer_event_start = plmt_timer_event_start,
	.timer_event_stop  = plmt_timer_event_stop,

	.system_reset	 = ae350_system_reset,

	.vendor_ext_provider = ae350_vendor_ext_provider
};

const struct sbi_platform platform = {
	.opensbi_version = OPENSBI_VERSION,
	.platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name = "Andes AE350",
	.features = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count = AE350_HART_COUNT,
	.hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr = (unsigned long)&platform_ops
};
