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
#include <sbi/sbi_platform.h>
#include <sbi/sbi_pmu.h>
#include <andes/andesv5.h>
#include <andes/andes_sbi.h>
#include <andes/pma.h>
#include <andes/pmu.h>
#include <andes/trigger.h>

struct smu_data smu = { 0 };

extern void __ae350_enable_coherency_warmboot(void);
extern void __ae350_disable_coherency(void);
static const struct sbi_hsm_device andes_smu = {
	.name = "andes_smu",
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

	rc = ae350_pmu_init(cold_boot);
	if (rc)
		return rc;

	if (!cold_boot)
		return 0;

	return 0;
}

static int ae350_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (!cold_boot)
		return 0;

	pma_init();
	ae350_hsm_device_init();
	trigger_init();
	fdt_cache_init();

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
