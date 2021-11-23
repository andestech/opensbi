/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#ifndef __SBI_CACHE_H__
#define __SBI_CACHE_H__

#include <sbi/sbi_types.h>
#include <sbi/sbi_hartmask.h>
#include <../platform/andes/ae350/cache.h>


/* clang-format on */

struct sbi_scratch;

struct sbi_cache_info {
	unsigned long start;
	unsigned long size;
	unsigned long dummy;
	unsigned long last_hartid;
	void (*local_fn)(struct sbi_cache_info *cinfo);
	struct sbi_hartmask smask;
};

#define SBI_CACHE_INFO_INIT(__p, __start, __size, __dummy, __last_hartid, __lfn, __src) \
do { \
	(__p)->start = (__start); \
	(__p)->size = (__size); \
	(__p)->dummy = 0; \
	(__p)->last_hartid = __last_hartid; \
	(__p)->local_fn = (__lfn); \
	SBI_HARTMASK_INIT_EXCEPT(&(__p)->smask, (__src)); \
} while (0)

void sbi_cache_invalidate_line(struct sbi_cache_info *cinfo);
void sbi_cache_invalidate_range(struct sbi_cache_info *cinfo);
void sbi_cache_wb_line(struct sbi_cache_info *cinfo);
void sbi_cache_wb_range(struct sbi_cache_info *cinfo);
void sbi_cache_wbinval_all(struct sbi_cache_info *cinfo);

#endif
