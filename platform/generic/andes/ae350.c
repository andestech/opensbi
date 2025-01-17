/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <platform_override.h>
#include <andes/andes_pmu.h>
#include <sbi_utils/cache/fdt_cache.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/sys/atcsmu.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_csr_detect.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_system.h>
#include <andes/andes_sbi.h>
#include <andes/andes.h>
#include <andes/andes_pma.h>
#include <andes/remoteproc.h>
#include <andes/trigger.h>

static struct smu_data smu = { 0 };
extern void _start_warm(void);

unsigned long save_regs_off;

static inline void ae350_disable_coherency(void)
{
	csr_write(CSR_MCCTLCOMMAND, MCCTL_L1D_WBINVAL_ALL);

	csr_clear(CSR_MCACHE_CTL, MCACHE_CTL_IC_EN | MCACHE_CTL_DC_EN);

	csr_clear(CSR_MCACHE_CTL, MCACHE_CTL_DC_COHEN_EN);

	while (csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA_EN);
}

static inline void ae350_enable_coherency(void)
{
	csr_set(CSR_MCACHE_CTL, MCACHE_CTL_DC_COHEN_EN);

	if (csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_DC_COHEN_EN)
		while (!(csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_DC_COHSTA_EN));

	csr_set(CSR_MCACHE_CTL, MCACHE_CTL_IC_EN | MCACHE_CTL_DC_EN);
}

static inline void ae350_enable_coherency_warmboot(void)
{
	ae350_enable_coherency();
	_start_warm();
}

static void ae350_suspend_non_ret_save(struct sbi_scratch *scratch, bool save_l2c_setting)
{
	struct save_regs *regs = sbi_scratch_offset_ptr(scratch, save_regs_off);
	struct sbi_trap_info trap = { 0 };
	unsigned long l2c_addr;

	regs->saved = true;

	regs->mcache_ctl	= csr_read_allowed(CSR_MCACHE_CTL, (ulong)&trap);
	regs->mcache_ctl2	= csr_read_allowed(CSR_MCACHE_CTL2, (ulong)&trap);
	regs->mmisc_ctl		= csr_read_allowed(CSR_MMISC_CTL, (ulong)&trap);
	regs->mpft_ctl		= csr_read_allowed(CSR_MPFT_CTL, (ulong)&trap);
	regs->mslideleg		= csr_read_allowed(CSR_MSLIDELEG, (ulong)&trap);
	regs->mxstatus		= csr_read_allowed(CSR_MXSTATUS, (ulong)&trap);

	regs->pmacfg0		= csr_read_allowed(CSR_PMACFG0, (ulong)&trap);
	regs->pmacfg1		= csr_read_allowed(CSR_PMACFG0 + 1, (ulong)&trap);
	regs->pmacfg2		= csr_read_allowed(CSR_PMACFG0 + 2, (ulong)&trap);
	regs->pmacfg3		= csr_read_allowed(CSR_PMACFG0 + 3, (ulong)&trap);
	regs->pmaaddr0		= csr_read_allowed(CSR_PMAADDR0, (ulong)&trap);
	regs->pmaaddr1		= csr_read_allowed(CSR_PMAADDR0 + 1, (ulong)&trap);
	regs->pmaaddr2		= csr_read_allowed(CSR_PMAADDR0 + 2, (ulong)&trap);
	regs->pmaaddr3		= csr_read_allowed(CSR_PMAADDR0 + 3, (ulong)&trap);
	regs->pmaaddr4		= csr_read_allowed(CSR_PMAADDR0 + 4, (ulong)&trap);
	regs->pmaaddr5		= csr_read_allowed(CSR_PMAADDR0 + 5, (ulong)&trap);
	regs->pmaaddr6		= csr_read_allowed(CSR_PMAADDR0 + 6, (ulong)&trap);
	regs->pmaaddr7		= csr_read_allowed(CSR_PMAADDR0 + 7, (ulong)&trap);
	regs->pmaaddr8		= csr_read_allowed(CSR_PMAADDR0 + 8, (ulong)&trap);
	regs->pmaaddr9		= csr_read_allowed(CSR_PMAADDR0 + 9, (ulong)&trap);
	regs->pmaaddr10		= csr_read_allowed(CSR_PMAADDR0 + 10, (ulong)&trap);
	regs->pmaaddr11		= csr_read_allowed(CSR_PMAADDR0 + 11, (ulong)&trap);
	regs->pmaaddr12		= csr_read_allowed(CSR_PMAADDR0 + 12, (ulong)&trap);
	regs->pmaaddr13		= csr_read_allowed(CSR_PMAADDR0 + 13, (ulong)&trap);
	regs->pmaaddr14		= csr_read_allowed(CSR_PMAADDR0 + 14, (ulong)&trap);
	regs->pmaaddr15		= csr_read_allowed(CSR_PMAADDR0 + 15, (ulong)&trap);

	regs->slie		= csr_read_allowed(CSR_SLIE, (ulong)&trap);
	regs->slip		= csr_read_allowed(CSR_SLIP, (ulong)&trap);

	if (save_l2c_setting && cache_get_addr(&l2c_addr) == SBI_OK)
		regs->l2c_ctl = readl((void*)(l2c_addr + L2C_CTL_OFFSET));
}

static void ae350_suspend_non_ret_restore(struct sbi_scratch *scratch, bool save_l2c_setting)
{
	struct save_regs *regs = sbi_scratch_offset_ptr(scratch, save_regs_off);
	struct sbi_trap_info trap = { 0 };
	unsigned long l2c_addr;

	if (regs->saved != true)
		return;

	csr_write_allowed(CSR_MCACHE_CTL, (ulong)&trap, regs->mcache_ctl);
	csr_write_allowed(CSR_MCACHE_CTL2, (ulong)&trap, regs->mcache_ctl2);
	csr_write_allowed(CSR_MMISC_CTL, (ulong)&trap, regs->mmisc_ctl);
	csr_write_allowed(CSR_MPFT_CTL, (ulong)&trap, regs->mpft_ctl);
	csr_write_allowed(CSR_MSLIDELEG, (ulong)&trap, regs->mslideleg);
	csr_write_allowed(CSR_MXSTATUS, (ulong)&trap, regs->mxstatus);

	csr_write_allowed(CSR_PMACFG0, (ulong)&trap, regs->pmacfg0);
	csr_write_allowed(CSR_PMACFG0 + 1, (ulong)&trap, regs->pmacfg1);
	csr_write_allowed(CSR_PMACFG0 + 2, (ulong)&trap, regs->pmacfg2);
	csr_write_allowed(CSR_PMACFG0 + 3, (ulong)&trap, regs->pmacfg3);
	csr_write_allowed(CSR_PMAADDR0, (ulong)&trap, regs->pmaaddr0);
	csr_write_allowed(CSR_PMAADDR0 + 1, (ulong)&trap, regs->pmaaddr1);
	csr_write_allowed(CSR_PMAADDR0 + 2, (ulong)&trap, regs->pmaaddr2);
	csr_write_allowed(CSR_PMAADDR0 + 3, (ulong)&trap, regs->pmaaddr3);
	csr_write_allowed(CSR_PMAADDR0 + 4, (ulong)&trap, regs->pmaaddr4);
	csr_write_allowed(CSR_PMAADDR0 + 5, (ulong)&trap, regs->pmaaddr5);
	csr_write_allowed(CSR_PMAADDR0 + 6, (ulong)&trap, regs->pmaaddr6);
	csr_write_allowed(CSR_PMAADDR0 + 7, (ulong)&trap, regs->pmaaddr7);
	csr_write_allowed(CSR_PMAADDR0 + 8, (ulong)&trap, regs->pmaaddr8);
	csr_write_allowed(CSR_PMAADDR0 + 9, (ulong)&trap, regs->pmaaddr9);
	csr_write_allowed(CSR_PMAADDR0 + 10, (ulong)&trap, regs->pmaaddr10);
	csr_write_allowed(CSR_PMAADDR0 + 11, (ulong)&trap, regs->pmaaddr11);
	csr_write_allowed(CSR_PMAADDR0 + 12, (ulong)&trap, regs->pmaaddr12);
	csr_write_allowed(CSR_PMAADDR0 + 13, (ulong)&trap, regs->pmaaddr13);
	csr_write_allowed(CSR_PMAADDR0 + 14, (ulong)&trap, regs->pmaaddr14);
	csr_write_allowed(CSR_PMAADDR0 + 15, (ulong)&trap, regs->pmaaddr15);

	csr_write_allowed(CSR_SLIE, (ulong)&trap, regs->slie);
	csr_write_allowed(CSR_SLIP, (ulong)&trap, regs->slip);

	if (save_l2c_setting && cache_get_addr(&l2c_addr) == SBI_OK)
		writel(regs->l2c_ctl, (void*)(l2c_addr + L2C_CTL_OFFSET));
}

static inline void wait_until_harts_sleep(u32 this_hart, u32 sleep_type, u32 sleep_status)
{
	u32 hart_cnt = sbi_platform_thishart_ptr()->hart_count;

	/* skip main hart */
	for (int hartid = 0; hartid < hart_cnt; hartid++)
		if ((hartid != this_hart) && (sleep_type == smu_get_sleep_type(&smu, hartid)))
			while (smu_check_pcs_status(&smu, sleep_status, hartid) != SBI_OK);
}

static int ae350_hart_start(u32 hartid, ulong saddr)
{
	u32 sleep_type = smu_get_sleep_type(&smu, hartid);
	/*
	 * Don't send wakeup command when:
	 * 1) boot-time
	 * 2) the target hart is non-sleepable 25-series hart0
	 * 3) deep sleep or light sleep
	 */
	if (!sbi_init_count(hartid) || (is_andes(25) && hartid == 0) ||
		sleep_type == SBI_SUSP_AE350_LIGHT_SLEEP || sleep_type == SBI_SUSP_AE350_DEEP_SLEEP)
		return sbi_ipi_raw_send(sbi_hartid_to_hartindex(hartid));

	/* Write wakeup command to the sleep hart only when resuming from hotplug */
	smu_set_command(&smu, WAKEUP_CMD, hartid);

	return 0;
}

static int ae350_hart_stop(void)
{
	int rc;
	u32 hartid = current_hartid();
	u32 sleep_type = smu_get_sleep_type(&smu, hartid);

	csr_write(CSR_SIE, 0);
	csr_write(CSR_MIE, 0);

	if (sleep_type == SBI_SUSP_AE350_LIGHT_SLEEP) {

		csr_write(CSR_MIE, MIP_MSIP);
		// set wake event (M-mode Software Interrupt only)
		smu_set_wakeup_events(&smu, 0x1 << PCS_WAKE_MSIP_OFFSET, hartid);

		smu_set_command(&smu, LIGHT_SLEEP_CMD, hartid);

		ae350_disable_coherency();

	} else if (sleep_type == SBI_SUSP_AE350_DEEP_SLEEP) {

		csr_write(CSR_MIE, MIP_MSIP);
		// set wake event (M-mode Software Interrupt only)
		smu_set_wakeup_events(&smu, 0x1 << PCS_WAKE_MSIP_OFFSET, hartid);

		smu_set_command(&smu, DEEP_SLEEP_CMD, hartid);

		rc = smu_set_reset_vector(&smu, (ulong)ae350_enable_coherency_warmboot,
					  hartid);
		if (rc)
			sbi_hart_hang();

		ae350_suspend_non_ret_save(sbi_scratch_thishart_ptr(), false);

		ae350_disable_coherency();

	} else {/* Hotplug */
		/**
		 * For Andes AX25MP, the hart0 shares power domain with
		 * L2-cache, instead of turning it off, it should fall
		 * through and jump to warmboot_addr.
		 */
		if (is_andes(25) && hartid == 0)
			return SBI_ENOTSUPP;
		/**
		 * disable all events, the current hart will be
		 * woken up from reset vector when other hart
		 * writes its PCS (power control slot) control
		 * register
		 */
		smu_set_wakeup_events(&smu, 0x0, hartid);

		smu_set_command(&smu, DEEP_SLEEP_CMD, hartid);

		rc = smu_set_reset_vector(&smu, (ulong)ae350_enable_coherency_warmboot,
					  hartid);
		if (rc)
			sbi_hart_hang();

		ae350_suspend_non_ret_save(sbi_scratch_thishart_ptr(), false);

		ae350_disable_coherency();
	}

	wfi();

	/* light sleep resume */
	ae350_enable_coherency();

	return SBI_ENOTSUPP;
}

static void ae350_hart_resume(void)
{
	if (save_regs_off)
		ae350_suspend_non_ret_restore(sbi_scratch_thishart_ptr(), true);
}

static const struct sbi_hsm_device andes_smu_hsm = {
	.name		= "andes_smu",
	.hart_start	= ae350_hart_start,
	.hart_stop	= ae350_hart_stop,
	.hart_resume	= ae350_hart_resume,
};

static int ae350_system_suspend_check(u32 sleep_type)
{
	return ((sleep_type == SBI_SUSP_AE350_LIGHT_SLEEP) ||
		(sleep_type == SBI_SUSP_AE350_DEEP_SLEEP)  ||
		(sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND)) ? SBI_OK : SBI_EINVAL;
}

static int ae350_system_suspend(u32 sleep_type, unsigned long mmode_resume_addr)
{
	u32 hartid = current_hartid();
	int rc;

	csr_write(CSR_SIE, 0);
	csr_write(CSR_MIE, 0);

	/* Peripheral interrupts are all wired to S-mode external interrupt */
	csr_set(CSR_SIE, MIP_SEIP);

	if (sleep_type == SBI_SUSP_AE350_LIGHT_SLEEP) {

		smu_set_command(&smu, LIGHT_SLEEP_CMD, hartid);

		wait_until_harts_sleep(hartid, sleep_type, LIGHT_SLEEP_STATUS);

		ae350_disable_coherency();

	} else if (sleep_type == SBI_SUSP_AE350_DEEP_SLEEP) {

		smu_set_command(&smu, DEEP_SLEEP_CMD, hartid);

		rc = smu_set_reset_vector(&smu, (ulong)ae350_enable_coherency_warmboot, hartid);
		if (rc)
			sbi_hart_hang();

		wait_until_harts_sleep(hartid, sleep_type, DEEP_SLEEP_STATUS);

		ae350_suspend_non_ret_save(sbi_scratch_thishart_ptr(), true);

		ae350_disable_coherency();
		/* disable L2 cache */
		cache_disable();
	}

	wfi();

	/* light sleep resume */
	ae350_enable_coherency();

	return SBI_OK;
}

static struct sbi_system_suspend_device andes_smu_susp = {
	.name			= "andes_smu",
	.system_suspend_check	= ae350_system_suspend_check,
	.system_suspend		= ae350_system_suspend,
};

static inline bool ae350_support_smepmp(void)
{
	unsigned long extensions[BITS_TO_LONGS(SBI_HART_EXT_MAX)] = { 0 };

	if (fdt_parse_isa_extensions(fdt_get_address(), current_hartid(), extensions))
		return false;

	if (__test_bit(SBI_HART_EXT_SMEPMP, extensions))
		return true;

	return false;
}

static void ae350_smu_device_init(void)
{
	int rc;
	void *fdt;

	fdt = fdt_get_address();

	rc = fdt_parse_compat_addr(fdt, (uint64_t *)&smu.addr,
				   "andestech,atcsmu");

	if (!rc) {
		sbi_hsm_set_device(&andes_smu_hsm);
		sbi_system_suspend_set_device(&andes_smu_susp);

		/** Quirk!
		 * The PMP entry is not enough on some Kavalan bitmaps
		 * (e.g., ax45mpv has only 8), so we use smepmp to
		 * determine if we need to setup pmp region for SMU device.
		 * Only Makatau ax65, ax66 bitmap support smepmp, and they
		 * have 16 PMP entries.
		 */
		if (ae350_support_smepmp())
			rc = sbi_domain_root_add_memrange(smu.addr, 0x1000, 0x1000,
							  SBI_DOMAIN_MEMREGION_MMIO |
							  SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW);

		/* Allocate space for regs that need to be saved/restored */
		save_regs_off = sbi_scratch_alloc_offset(sizeof(struct save_regs));
		if (!save_regs_off)
			sbi_hart_hang();
	}
}

static int ae350_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (!cold_boot)
		return 0;

	pma_init();
	trigger_init();

	return 0;
}

static int ae350_early_init(bool cold_boot, const struct fdt_match *match)
{
	if (cold_boot) {
		remoteproc_init(cold_boot);
		ae350_smu_device_init();
		return fdt_cache_init();
	}

	remoteproc_init(cold_boot);

	if (save_regs_off)
		ae350_suspend_non_ret_restore(sbi_scratch_thishart_ptr(), false);

	return 0;
}

static const struct fdt_match andes_ae350_match[] = {
	{ .compatible = "andestech,ae350" },
	{ },
};

const struct platform_override andes_ae350 = {
	.match_table = andes_ae350_match,
	.early_init  = ae350_early_init,
	.final_init  = ae350_final_init,
	.extensions_init = andes_pmu_extensions_init,
	.pmu_init = andes_pmu_init,
	.vendor_ext_provider = andes_sbi_vendor_ext_provider,
};
