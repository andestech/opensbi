// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 *
 */
#include <andes/andes.h>
#include <andes/andes_sbi.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_error.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include <andes/trigger.h>

enum sbi_ext_andes_fid {
	SBI_EXT_ANDES_FID0 = 0, /* Reserved for future use */
	SBI_EXT_ANDES_IOCP_SW_WORKAROUND,
	SBI_EXT_ANDES_TRIGGER_SET = ANDES_SBI_INTERNAL_FID_START,
};

static bool andes_cache_controllable(void)
{
	return (((csr_read(CSR_MICM_CFG) & MICM_CFG_ISZ_MASK) ||
		 (csr_read(CSR_MDCM_CFG) & MDCM_CFG_DSZ_MASK)) &&
		(csr_read(CSR_MMSC_CFG) & MMSC_CFG_CCTLCSR_MASK) &&
		(csr_read(CSR_MCACHE_CTL) & MCACHE_CTL_CCTL_SUEN_MASK) &&
		misa_extension('U'));
}

static bool andes_iocp_disabled(void)
{
	return (csr_read(CSR_MMSC_CFG) & MMSC_IOCP_MASK) ? false : true;
}

static bool andes_apply_iocp_sw_workaround(void)
{
	return andes_cache_controllable() & andes_iocp_disabled();
}

int andes_sbi_vendor_ext_provider(long funcid,
				  struct sbi_trap_regs *regs,
				  struct sbi_ecall_return *out,
				  const struct fdt_match *match)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_ANDES_IOCP_SW_WORKAROUND:
		out->value = andes_apply_iocp_sw_workaround();
		break;
	case SBI_EXT_ANDES_TRIGGER_SET:
		out->value = mcall_set_trigger(regs->a0, regs->a1, 0, 0, regs->a2);
		break;

	default:
		sbi_panic("%s(): funcid: %#lx is not supported\n", __func__, funcid);
		break;
	}

	return ret;
}
