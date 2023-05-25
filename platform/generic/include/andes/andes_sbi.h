/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2023 Renesas Electronics Corp.
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Lad Prabhakar <prabhakar.mahadev-lad.rj@bp.renesas.com>
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#ifndef _ANDES_SBI_H_
#define _ANDES_SBI_H_

#include <sbi/sbi_trap.h>
#include <sbi_utils/fdt/fdt_helper.h>

enum sbi_ext_andes_fid {
	SBI_EXT_ANDES_GET_MCACHE_CTL_STATUS = 0,
	SBI_EXT_ANDES_GET_MMISC_CTL_STATUS,
	SBI_EXT_ANDES_SET_MCACHE_CTL,
	SBI_EXT_ANDES_SET_MMISC_CTL,
	SBI_EXT_ANDES_ICACHE_OP,
	SBI_EXT_ANDES_DCACHE_OP,
	SBI_EXT_ANDES_L1CACHE_I_PREFETCH,
	SBI_EXT_ANDES_L1CACHE_D_PREFETCH,
	SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE,
	SBI_EXT_ANDES_WRITE_AROUND,
	SBI_EXT_ANDES_TRIGGER,
	SBI_EXT_ANDES_SET_PFM,
	SBI_EXT_ANDES_READ_POWERBRAKE,
	SBI_EXT_ANDES_WRITE_POWERBRAKE,
	SBI_EXT_ANDES_SUSPEND_PREPARE,
	SBI_EXT_ANDES_SUSPEND_MEM,
	SBI_EXT_ANDES_SET_SUSPEND_MODE,
	SBI_EXT_ANDES_ENTER_SUSPEND_MODE,
	SBI_EXT_ANDES_RESTART,
	SBI_EXT_ANDES_RESET_VEC,
	SBI_EXT_ANDES_SET_PMA,
	SBI_EXT_ANDES_FREE_PMA,
	SBI_EXT_ANDES_PROBE_PMA,
	SBI_EXT_ANDES_DCACHE_WBINVAL_ALL,
	SBI_EXT_ANDES_HPM,
};

int ae350_set_suspend_mode(u32 suspend_mode);
int ae350_enter_suspend_mode(bool main_core, unsigned int wake_mask);

int andes_sbi_vendor_ext_provider(long extid, long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_value,
				  struct sbi_trap_info *out_trap,
				  const struct fdt_match *match);

#endif /* _ANDES_SBI_H_ */
