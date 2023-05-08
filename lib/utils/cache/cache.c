/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 *
 * derivate: lib/utils/gpio/gpio.c
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_error.h>
#include <sbi_utils/cache/cache.h>

static const struct cache *cache = NULL;

int cache_add(struct cache *c)
{
	if (!c)
		return SBI_EINVAL;

	cache = c;
	return 0;
}

int cache_enable(void)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->enable)
		return SBI_ENOSYS;

	return cache->enable();
}

int cache_disable(void)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->disable)
		return SBI_ENOSYS;

	return cache->disable();
}

int cache_wbinval_all(void)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->wbinval_all)
		return SBI_ENOSYS;

	return cache->wbinval_all();
}
