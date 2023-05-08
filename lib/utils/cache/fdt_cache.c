/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/cache/fdt_cache.h>

/* List of FDT cache drivers generated at compile time */
extern struct fdt_cache *fdt_cache_drivers[];
extern unsigned long fdt_cache_drivers_size;

int fdt_cache_driver_init(void *fdt, struct fdt_cache *drv)
{
	int noff, rc = SBI_ENODEV;
	const struct fdt_match *match;

	noff = fdt_find_match(fdt, -1, drv->match_table, &match);
	if (noff < 0)
		return SBI_ENODEV;

	if (drv->init) {
		rc = drv->init(fdt, noff, match);
		if (rc && rc != SBI_ENODEV) {
			sbi_printf("%s: %s init failed, %d\n",
				   __func__, match->compatible, rc);
		}
	}

	return rc;
}

void fdt_cache_init(void)
{
	int pos;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < fdt_cache_drivers_size; pos++)
		fdt_cache_driver_init(fdt, fdt_cache_drivers[pos]);
}
