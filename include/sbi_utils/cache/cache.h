/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <sbi/sbi_types.h>

/** Representation of a cache controller */
struct cache {
	/**
	 * Enable cache
	 *
	 * @return 0 on success and negative error code on failure
	 */
	int (*enable)(void);
	/**
	 * Disable cache
	 *
	 * @return 0 on success and negative error code on failure
	 */
	int (*disable)(void);
	/**
	 * Write back invalidate all cache lines
	 *
	 * @return 0 on success and negative error code on failure
	 */
	int (*wbinval_all)(void);

	/*
	 * L2C HPM helpers (Andes-specific)
	 */
	int (*read_hpm_ctr)(u64 *);
	int (*write_hpm_ctr)(u64);
	int (*start_hpm)(u32);
	int (*stop_hpm)(void);
	bool (*hpm_idle)(void);
};

/** Register a cache controller */
int cache_add(struct cache *c);

/** Enable cache */
int cache_enable(void);

/** Enable cache */
int cache_disable(void);

/** Write back invalidate all cache lines */
int cache_wbinval_all(void);

/*
 * L2C HPM helpers (Andes-specific)
 */
int cache_read_hpm_ctr(u64 *);
int cache_write_hpm_ctr(u64);
int cache_start_hpm(u32);
int cache_stop_hpm(void);
bool cache_hpm_idle(void);

#endif /* __CACHE_H__ */
