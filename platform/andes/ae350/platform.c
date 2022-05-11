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
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_trap.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <libfdt.h>
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
extern int ae350_suspend_mode;
extern struct sbi_platform platform;

unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
				unsigned long arg2, unsigned long arg3,
				unsigned long arg4)
{
	void *fdt = (void *)arg1;
	u32 max_hartid;
	int root_offset, cpus_offset;

	root_offset = fdt_path_offset(fdt, "/");
	if (root_offset < 0)
		goto fail;

	/* Get hart count */
	cpus_offset = fdt_parse_max_hart_id(fdt, &max_hartid);
	if (cpus_offset < 0)
		goto fail;

	platform.hart_count = max_hartid + 1;

	// Return original FDT pointer
	return arg1;

fail:
	while (1)
		wfi();
}

/* Platform final initialization. */
static int ae350_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = fdt_get_address();
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

static uintptr_t mcall_set_pfm()
{
	csr_clear(CSR_SLIP, MIP_SOVFIP);
	csr_set(CSR_MIE, MIP_MOVFIP);
	return 0;
}

static uintptr_t mcall_suspend_prepare(char main_core, char enable)
{
	smu_suspend_prepare(main_core,enable);
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
	__builtin_unreachable();
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

static struct sbi_ipi_device plicsw_ipi = {
	.name = "ae350_plicsw",
	.ipi_send = plicsw_ipi_send,
	.ipi_clear = plicsw_ipi_clear
};

/* Initialize IPI for current HART. */
static int ae350_ipi_init(bool cold_boot)
{
	int ret;
	u32 hart_count = sbi_platform_thishart_ptr()->hart_count;

	if (cold_boot) {
		ret = plicsw_cold_ipi_init(AE350_PLICSW_ADDR,
					   hart_count);
		if (ret)
			return ret;

		sbi_ipi_set_device(&plicsw_ipi);
	}

	return plicsw_warm_ipi_init();
}

/* Initialize platform timer for current HART. */
static int ae350_timer_init(bool cold_boot)
{
	int ret;
	u32 hart_count = sbi_platform_thishart_ptr()->hart_count;

	if (cold_boot) {
		ret = plmt_cold_timer_init(AE350_PLMT_ADDR,
					   hart_count);
		if (ret)
			return ret;
	}

	return plmt_warm_timer_init();
}

/* called flow:

	1. kernel -> ae350_set_suspend_mode(light/deep) -> set variable: ae350_suspend_mode

	2. cpu_stop() -> sbi_hsm_hart_stop() -> sbi_hsm_exit() ->
		jump_warmboot() -> sbi_hsm_hart_wait() -> ae350_enter_suspend_mode() -> normal/light/deep */
int ae350_set_suspend_mode(int suspend_mode)
{
	ae350_suspend_mode = suspend_mode;

	if(suspend_mode == LightSleepMode){
		sbi_printf("ae350_suspend_mode: Light Sleep Mode\n");
	}else if(suspend_mode == DeepSleepMode){
		sbi_printf("ae350_suspend_mode: Deep Sleep Mode\n");
	}
	return 0;
}

int ae350_enter_suspend_mode(int suspend_mode){
	u32 hartid = current_hartid();

	// smu function
	if(suspend_mode == LightSleepMode){
		// set SMU wakeup enable & MISC control
		smu_set_wakeup_enable(hartid, 1 << PCS_WAKE_MSIP_OFF);
		// Disable higher privilege's non-wakeup event
		smu_suspend_prepare(false, false);
		// set SMU light sleep command
		smu_set_sleep(hartid, LightSleep_CTL);
		// D-cache disable
		mcall_dcache_op(0);
		// wait for interrupt
		wfi();
		// D-cache enable
		mcall_dcache_op(1);
		// enable privilege
		smu_suspend_prepare(false, true);
	}else if(suspend_mode == DeepSleepMode){
		// set SMU wakeup enable & MISC control
		smu_set_wakeup_enable(hartid, 1 << PCS_WAKE_MSIP_OFF);
		// Disable higher privilege's non-wakeup event
		smu_suspend_prepare(false, false);
		// set SMU Deep sleep command
		smu_set_sleep(hartid, DeepSleep_CTL);
		// stop & wfi & resume
		cpu_suspend2ram();
		// enable privilege
		smu_suspend_prepare(false, true);
	}

	return 0;
}

/* Vendor-Specific SBI handler */
static int ae350_vendor_ext_provider(long extid, long funcid,
	const struct sbi_trap_regs *regs, unsigned long *out_value,
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
		ret = mcall_set_mcache_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_SET_MMISC_CTL:
		ret = mcall_set_mmisc_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_ICACHE_OP:
		ret = mcall_icache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_DCACHE_OP:
		ret = mcall_dcache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_L1CACHE_I_PREFETCH:
		ret = mcall_l1_cache_i_prefetch_op(regs->a0);
		break;
	case SBI_EXT_ANDES_L1CACHE_D_PREFETCH:
		ret = mcall_l1_cache_d_prefetch_op(regs->a0);
		break;
	case SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE:
		ret = mcall_non_blocking_load_store(regs->a0);
		break;
	case SBI_EXT_ANDES_WRITE_AROUND:
		ret = mcall_write_around(regs->a0);
		break;
	case SBI_EXT_ANDES_TRIGGER:
		*out_value = mcall_set_trigger(regs->a0, regs->a1, 0, 0, regs->a2);
		break;
	case SBI_EXT_ANDES_SET_PFM:
		ret = mcall_set_pfm();
		break;
	case SBI_EXT_ANDES_READ_POWERBRAKE:
		*out_value = csr_read(CSR_MPFT_CTL);
		break;
	case SBI_EXT_ANDES_WRITE_POWERBRAKE:
		csr_write(CSR_MPFT_CTL, regs->a0);
		break;
	case SBI_EXT_ANDES_SUSPEND_PREPARE:
		ret = mcall_suspend_prepare(regs->a0, regs->a1);
		break;
	case SBI_EXT_ANDES_SUSPEND_MEM:
		ret = mcall_suspend_backup();
		break;
	case SBI_EXT_ANDES_SET_SUSPEND_MODE:
		ae350_set_suspend_mode(regs->a0);
		break;
	case SBI_EXT_ANDES_RESTART:
		mcall_restart(regs->a0);
		break;
	case SBI_EXT_ANDES_RESET_VEC:
		mcall_set_reset_vec(regs->a0);
		break;
	case SBI_EXT_ANDES_SET_PMA:
		mcall_set_pma(regs->a0, regs->a1, regs->a2, regs->a3);
		break;
	case SBI_EXT_ANDES_FREE_PMA:
		mcall_free_pma(regs->a0);
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
	.final_init = ae350_final_init,

	.console_init = ae350_console_init,

	.irqchip_init = ae350_irqchip_init,

	.ipi_init     = ae350_ipi_init,

	.timer_init	   = ae350_timer_init,

	.vendor_ext_provider = ae350_vendor_ext_provider
};

struct sbi_platform platform = {
	.opensbi_version = OPENSBI_VERSION,
	.platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name = "Andes AE350",
	.features = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count = AE350_HART_COUNT_MAX,
	.hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr = (unsigned long)&platform_ops
};
