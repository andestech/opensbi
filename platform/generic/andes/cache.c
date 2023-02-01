/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Nylon Chen <nylon7@andestech.com>
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_types.h>
#include <andes/cache.h>
#include <andes/andesv5.h>

__always_inline void mcall_set_mcache_ctl(unsigned long val)
{
	csr_write(CSR_MCACHE_CTL, val);
}

__always_inline void mcall_set_mmisc_ctl(unsigned long val)
{
	csr_write(CSR_MMISC_CTL, val);
}

__always_inline void mcall_dcache_wbinval_all(void)
{
	csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_WBINVAL_ALL);
}
