// SPDX-License-Identifier: BSD-2-Clause

#ifndef _RISCV_ANDES_SBI_H
#define _RISCV_ANDES_SBI_H

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_trap.h>
#include <sbi_utils/fdt/fdt_helper.h>

/*
 * Set Andes's internal SBI Call FID start at 64 to avoid the impact of
 * upstream add or remove in sbi_ext_andes_fid{}. If the FID numbers
 * change, our released OpenSBI will lose backward compatible.
 */
#define ANDES_SBI_INTERNAL_FID_START     64

int andes_sbi_vendor_ext_provider(long funcid,
				  struct sbi_trap_regs *regs,
				  struct sbi_ecall_return *out,
				  const struct fdt_match *match);

#endif /* _RISCV_ANDES_SBI_H */
