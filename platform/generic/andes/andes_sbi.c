/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Lad Prabhakar <prabhakar.mahadev-lad.rj@bp.renesas.com>
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_types.h>
#include <andes/pma.h>
#include <andes/trigger.h>
#include <andes/andes_sbi.h>
#include <andes/andesv5.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/sys/atcsmu.h>

extern void __ae350_disable_coherency(void);
extern void __ae350_enable_coherency(void);
extern void cpu_suspend2ram(void);

static __always_inline void mcall_set_mcache_ctl(unsigned long val)
{
	csr_write(CSR_MCACHE_CTL, val);
}

static __always_inline void mcall_set_mmisc_ctl(unsigned long val)
{
	csr_write(CSR_MMISC_CTL, val);
}

static __always_inline void mcall_dcache_wbinval_all(void)
{
	int rc;

	csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_WBINVAL_ALL);
	rc = cache_wbinval_all(); /* L2C wbinval all */
	if (rc)
		sbi_printf("%s: WARN: L2-cache wbinval_all failed\n", __func__);
}

static __always_inline void mcall_dcache_op(unsigned int enable)
{
	int rc;

	if (enable) {
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_DC_EN);
		rc = cache_enable(); /* L2C enable */
		if (rc)
			sbi_printf("%s: WARN: L2-cache enable failed\n", __func__);
	} else {
		sbi_printf(
			"%s: WARN: The use of 'disable d-cache' is deprecated.\n",
			__func__);
	}
}

/*
 * AST520, AST530-beta called flow:
 *
 * 1. kernel -> ae350_set_suspend_mode(light/deep) -> set variable: ae350_suspend_mode
 *
 * 2. cpu_stop() -> sbi_hsm_hart_stop() -> sbi_hsm_exit() ->
 *     jump_warmboot() -> sbi_hsm_hart_wait() -> ae350_enter_suspend_mode() -> normal/light/deep
 */

int ae350_set_suspend_mode(u32 suspend_mode)
{
	u32 hartid = current_hartid();

	ae350_suspend_mode[hartid] = suspend_mode;
	return 0;
}

int ae350_enter_suspend_mode(bool main_core, unsigned int wake_mask)
{
	u32 hartid, cpu_nums, suspend_mode;

	hartid	 = current_hartid();
	cpu_nums = sbi_platform_thishart_ptr()->hart_count;

	suspend_mode = ae350_suspend_mode[hartid];

	if (!smu.addr) {
		switch (suspend_mode) {
		case LightSleepMode:
		case DeepSleepMode:
			if (main_core)
				sbi_printf(
					"%s(): LightSleep and DeepSleep are not supported.\n",
					__func__);
		case CpuHotplugDeepSleepMode:
			/*
			 * The hart skips deep sleep and waiting in
			 * hsm start pending state.
			 */
		default:
			goto go_resume;
			break;
		}
	}

	switch (suspend_mode) {
	case LightSleepMode:
		/*
		 * This print is commented out to prevent system hang. (Ref. Bugzilla 27318)
		 *
		 * System will hang after light sleep being executed multiple times.
		 * This is because main hart and other harts do not enter sleep mode (WFI)
		 * at the same time, and UART interrupt could cause hart to resume execution 
		 * from the WFI (the one used to enter sleep mode in OpenSBI) immediately after
		 * it was executed. Then when executing the WFI in kernel's idle routine, 
		 * the system will enter sleep mode and could never be woken up again.
		 *
		 * Comment out this print to prevent system hang.
		 */
		// sbi_printf("%s(): CPU[%d] LightSleepMode\n", __func__, hartid);

		// set SMU wakeup enable & MISC control
		smu_set_wakeup_enable(hartid, main_core, wake_mask);
		// Disable higher privilege's non-wakeup event
		smu_suspend_prepare(main_core, false);
		// set SMU light sleep command
		smu_set_sleep(hartid, LIGHT_SLEEP_MODE);
		// Wait for other cores to enter sleeping mode
		if (main_core)
			smu_check_pcs_status(LIGHT_SLEEP_STATUS, cpu_nums);
		// D-cache disable
		__ae350_disable_coherency();
		// wait for interrupt
		wfi();
		// D-cache enable
		__ae350_enable_coherency();
		// enable privilege
		smu_suspend_prepare(main_core, true);
		break;
	case DeepSleepMode:
		sbi_printf("%s(): CPU[%d] DeepSleepMode\n", __func__, hartid);

		// set SMU wakeup enable & MISC control
		smu_set_wakeup_enable(hartid, main_core, wake_mask);
		// Disable higher privilege's non-wakeup event
		smu_suspend_prepare(main_core, false);
		// set SMU Deep sleep command
		smu_set_sleep(hartid, DEEP_SLEEP_MODE);
		// Wait for other cores to enter sleeping mode
		if (main_core)
			smu_check_pcs_status(DEEP_SLEEP_STATUS, cpu_nums);
		// stop & wfi & resume
		cpu_suspend2ram();
		// enable privilege
		smu_suspend_prepare(main_core, true);
		break;
	case CpuHotplugDeepSleepMode:
		/*
		 * In 25-series, core 0 is binding with L2 power domain,
		 * core 0 should NOT enter deep sleep mode.
		 *
		 * For 45-series, every core has its own power domain,
		 * It's ok to sleep main core.
		 */
		if (!is_andes(25) || !(hartid == 0)) {
			sbi_printf("%s(): CPU[%d] Cpu Hotplug DeepSleepMode\n",
				   __func__, hartid);

			// set SMU wakeup enable & MISC control
			smu_set_wakeup_enable(hartid, main_core, 0);
			// Disable higher privilege's non-wakeup event
			smu_suspend_prepare(-1, false);
			// set SMU Deep sleep command
			smu_set_sleep(hartid, DEEP_SLEEP_MODE);
			// stop & wfi & resume
			cpu_suspend2ram();
			// enable privilege
			smu_suspend_prepare(-1, true);
		}
		break;
	default:
		sbi_printf("%s(): CPU[%d] Unsupported ae350 suspend mode\n",
			   __func__, hartid);
		sbi_hart_hang();
	}

go_resume:
	// reset suspend mode to NormalMode (active)
	ae350_suspend_mode[hartid] = NormalMode;

	return 0;
}

int andes_sbi_vendor_ext_provider(long extid, long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_value,
				  struct sbi_trap_info *out_trap,
				  const struct fdt_match *match)
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
		mcall_set_mcache_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_SET_MMISC_CTL:
		mcall_set_mmisc_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_DCACHE_WBINVAL_ALL:
		mcall_dcache_wbinval_all();
		break;
	case SBI_EXT_ANDES_DCACHE_OP:
		mcall_dcache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_ICACHE_OP:
	case SBI_EXT_ANDES_L1CACHE_I_PREFETCH:
	case SBI_EXT_ANDES_L1CACHE_D_PREFETCH:
	case SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE:
	case SBI_EXT_ANDES_WRITE_AROUND:
	case SBI_EXT_ANDES_SET_PFM:
	case SBI_EXT_ANDES_SUSPEND_PREPARE:
		sbi_panic("%s(): deprecated cache SBI call (funcid: %#lx)\n",
			  __func__, funcid);
		break;
	case SBI_EXT_ANDES_TRIGGER:
		*out_value = mcall_set_trigger(regs->a0, regs->a1, 0, 0, regs->a2);
		break;
	case SBI_EXT_ANDES_READ_POWERBRAKE:
		*out_value = csr_read(CSR_MPFT_CTL);
		break;
	case SBI_EXT_ANDES_WRITE_POWERBRAKE:
		csr_write(CSR_MPFT_CTL, regs->a0);
		break;
	case SBI_EXT_ANDES_SET_SUSPEND_MODE:
		ae350_set_suspend_mode(regs->a0);
		break;
	case SBI_EXT_ANDES_ENTER_SUSPEND_MODE:
		ae350_enter_suspend_mode(regs->a0, regs->a1);
		break;
	case SBI_EXT_ANDES_SET_PMA:
		ret = mcall_set_pma(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_ANDES_FREE_PMA:
		ret = mcall_free_pma(regs->a0);
		break;
	case SBI_EXT_ANDES_PROBE_PMA:
		*out_value = mcall_probe_pma();
		break;
	case SBI_EXT_ANDES_HPM:
		*out_value = andes_hpm();
		break;
	default:
		sbi_panic("%s(): funcid: %#lx is not supported\n", __func__, funcid);
		break;
	}

	return ret;
}
