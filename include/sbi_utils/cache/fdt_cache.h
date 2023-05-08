/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#ifndef __FDT_CACHE_H__
#define __FDT_CACHE_H__

struct fdt_cache {
	const struct fdt_match *match_table;
	int (*init)(void *fdt, int nodeoff, const struct fdt_match *match);
};

#ifdef CONFIG_FDT_CACHE

/**
 * fdt_cache_driver_init() - initialize cache driver based on the device-tree
 */
int fdt_cache_driver_init(void *fdt, struct fdt_cache *drv);

/**
 * fdt_cache_init() - initialize cache drivers based on the device-tree
 *
 * This function shall be invoked in final init.
 */
void fdt_cache_init(void);

#else

static inline int fdt_cache_driver_init(void *fdt, struct fdt_cache *drv)
{
	return 0;
}
static inline void fdt_cache_init(void) { }

#endif /* CONFIG_FDT_CACHE */

#endif /* __FDT_CACHE_H__ */
