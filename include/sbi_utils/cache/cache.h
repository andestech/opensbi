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

#ifndef __ASSEMBLER__

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
	/**
	 * Get cache base address
	 *
	 * @return NULL if cache does not exist, else return cache base address
	 */
	int (*get_addr)(unsigned long *addr);
};

/** Register a cache controller */
int cache_add(struct cache *c);

/** Enable cache */
int cache_enable(void);

/** Enable cache */
int cache_disable(void);

/** Write back invalidate all cache lines */
int cache_wbinval_all(void);

/** Get the base address of cache */
int cache_get_addr(unsigned long *addr);

#endif /* __ASSEMBLER__ */

#endif /* __CACHE_H__ */
