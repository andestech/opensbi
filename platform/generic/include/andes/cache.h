/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *   Nylon Chen <nylon7@andestech.com>
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#ifndef _ANDES_CACHE_H_
#define _ANDES_CACHE_H_

void mcall_set_mcache_ctl(unsigned long val);
void mcall_set_mmisc_ctl(unsigned long val);
void mcall_dcache_wbinval_all(void);

#endif /* _ANDES_CACHE_H_ */
