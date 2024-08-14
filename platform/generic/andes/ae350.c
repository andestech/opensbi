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
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/sys/atcsmu.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_system.h>
#include <andes/andes_sbi.h>
#include <andes/andes.h>
#include <andes/andes_pma.h>
#include <andes/trigger.h>

static struct smu_data smu = { 0 };
extern void _start_warm(void);

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

static int ae350_hart_start(u32 hartid, ulong saddr)
{
	/*
	 * Don't send wakeup command when:
	 * 1) boot-time
	 * 2) the target hart is non-sleepable 25-series hart0
	 */
	if (!sbi_init_count(hartid) || (is_andes(25) && hartid == 0))
		return sbi_ipi_raw_send(sbi_hartid_to_hartindex(hartid));

	/* Write wakeup command to the sleep hart */
	smu_set_command(&smu, WAKEUP_CMD, hartid);

	return 0;
}

static int ae350_hart_stop(void)
{
	int rc;
	u32 hartid = current_hartid();

	/**
	 * For Andes AX25MP, the hart0 shares power domain with
	 * L2-cache, instead of turning it off, it should fall
	 * through and jump to warmboot_addr.
	 */
	if (is_andes(25) && hartid == 0)
		return SBI_ENOTSUPP;

	if (!smu_support_sleep_mode(&smu, DEEPSLEEP_MODE, hartid))
		return SBI_ENOTSUPP;

	/**
	 * disable all events, the current hart will be
	 * woken up from reset vector when other hart
	 * writes its PCS (power control slot) control
	 * register
	 */
	smu_set_wakeup_events(&smu, 0x0, hartid);
	smu_set_command(&smu, DEEP_SLEEP_CMD, hartid);

	rc = smu_set_reset_vector(&smu,
				  (ulong)ae350_enable_coherency_warmboot,
				  hartid);
	if (rc)
		goto fail;

	ae350_disable_coherency();

	wfi();

fail:
	/* It should never reach here */
	sbi_hart_hang();
	return 0;
}

static void ae350_hart_resume(void)
{
	return;
}

static const struct sbi_hsm_device andes_smu_hsm = {
	.name		= "andes_smu",
	.hart_start	= ae350_hart_start,
	.hart_stop	= ae350_hart_stop,
	.hart_resume	= ae350_hart_resume,
};

static int ae350_system_suspend_check(u32 sleep_type)
{
	return 0;
}

static int ae350_system_suspend(u32 sleep_type, unsigned long mmode_resume_addr)
{
	return 0;
}

static struct sbi_system_suspend_device andes_smu_susp = {
	.name			= "andes_smu",
	.system_suspend_check	= ae350_system_suspend_check,
	.system_suspend		= ae350_system_suspend,
};

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
	}
}

static int ae350_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (!cold_boot)
		return 0;

	pma_init();
	ae350_smu_device_init();
	trigger_init();

	return 0;
}

static int ae350_early_init(bool cold_boot, const struct fdt_match *match)
{
	if (cold_boot)
		return fdt_cache_init();

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
