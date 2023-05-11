/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <platform_override.h>
#include <sbi_utils/cache/fdt_cache.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/sys/atcsmu.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_pmu.h>
#include <andes/andesv5.h>
#include <andes/andes_sbi.h>
#include <andes/pma.h>
#include <andes/pmu.h>
#include <andes/l2c_hpm.h>
#include <andes/trigger.h>

static struct smu_data smu = { 0 };
extern void __ae350_enable_coherency_warmboot(void);
extern void __ae350_disable_coherency(void);

static int ae350_hart_start(u32 hartid, ulong saddr)
{
	/* Don't send wakeup command at boot-time */
	/* Andes AX25MP hart0 shares power domain with L2-cache, simply send ipi to wakeup hart0 */
	if (!sbi_init_count(hartid) || (is_andes(25) && hartid == 0))
		return sbi_ipi_raw_send(hartid);

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

	rc = smu_set_reset_vector(&smu, (ulong)__ae350_enable_coherency_warmboot,
			       hartid);
	if (rc)
		goto fail;

	__ae350_disable_coherency();

	wfi();

fail:
	/* It should never reach here */
	sbi_hart_hang();
	return 0;
}

static const struct sbi_hsm_device andes_smu = {
	.name	      = "andes_smu",
	.hart_start   = ae350_hart_start,
	.hart_stop    = ae350_hart_stop,
};

static void ae350_hsm_device_init(void)
{
	int rc;
	void *fdt;

	fdt = fdt_get_address();

	rc = fdt_parse_compat_addr(fdt, (uint64_t *)&smu.addr,
				   "andestech,atcsmu");

	if (!rc) {
		sbi_hsm_set_device(&andes_smu);
	}
}

static int ae350_early_init(bool cold_boot, const struct fdt_match *match)
{
	int rc;

	if (!cold_boot)
		return 0;

	rc = ae350_fdt_add_pmu();
	if (rc)
		return rc;

	return 0;
}

const struct sbi_pmu_device ae350_pmu = {
	.name			= "andes_pmu",
	.fw_event_validate_code = ae350_fw_event_validate_code,
	.fw_counter_match_code	= ae350_fw_counter_match_code,
	.fw_counter_read_value	= ae350_fw_counter_read_value,
	.fw_counter_start	= ae350_fw_counter_start,
	.fw_counter_stop	= ae350_fw_counter_stop,
};

static int ae350_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (!cold_boot)
		return 0;

	pma_init();
	ae350_hsm_device_init();
	trigger_init();
	fdt_cache_init();
	sbi_pmu_set_device(&ae350_pmu);

	return 0;
}

static const struct fdt_match andes_ae350_match[] = {
	{ .compatible = "andestech,ae350" },
	{ .compatible = "andestech,ax25" },
	{ .compatible = "andestech,a25" },
	{ },
};

const struct platform_override andes_ae350 = {
	.match_table = andes_ae350_match,
	.early_init  = ae350_early_init,
	.final_init  = ae350_final_init,
	.vendor_ext_provider = andes_sbi_vendor_ext_provider,
};
