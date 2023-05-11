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

/*
 * L2C HPM helpers (Andes-specific)
 */
int cache_read_hpm_ctr(u64 *out_val)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->read_hpm_ctr)
		return SBI_ENOSYS;

	return cache->read_hpm_ctr(out_val);
}

int cache_write_hpm_ctr(u64 val)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->write_hpm_ctr)
		return SBI_ENOSYS;

	return cache->write_hpm_ctr(val);
}

int cache_start_hpm(u32 event_idx_code)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->start_hpm)
		return SBI_ENOSYS;

	return cache->start_hpm(event_idx_code);
}

int cache_stop_hpm(void)
{
	if (!cache)
		return SBI_EINVAL;
	if (!cache->stop_hpm)
		return SBI_ENOSYS;

	return cache->stop_hpm();
}

bool cache_hpm_idle(void)
{
	if (!cache)
		return false;
	if (!cache->hpm_idle)
		return false;

	return cache->hpm_idle();
}
