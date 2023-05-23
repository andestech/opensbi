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

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_types.h>
#include <andes/pma.h>
#include <andes/trigger.h>
#include <andes/andes_sbi.h>
#include <andes/andesv5.h>
#include <sbi_utils/cache/cache.h>

static __always_inline void mcall_set_mcache_ctl(unsigned long val)
{
	csr_write(CSR_MCACHE_CTL, val);
}

static __always_inline void mcall_set_mmisc_ctl(unsigned long val)
{
	csr_write(CSR_MMISC_CTL, val);
}

static __always_inline void mcall_dcache_wbinval_all(void)
{
	int rc;

	csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_WBINVAL_ALL);
	rc = cache_wbinval_all(); /* L2C wbinval all */
	if (rc)
		sbi_printf("%s: WARN: L2-cache wbinval_all failed\n", __func__);
}

static __always_inline void mcall_dcache_op(unsigned int enable)
{
	int rc;

	if (enable) {
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_DC_EN);
		rc = cache_enable(); /* L2C enable */
		if (rc)
			sbi_printf("%s: WARN: L2-cache enable failed\n", __func__);
	} else {
		sbi_printf(
			"%s: WARN: The use of 'disable d-cache' is deprecated.\n",
			__func__);
	}
}

int andes_sbi_vendor_ext_provider(long extid, long funcid,
				  const struct sbi_trap_regs *regs,
				  unsigned long *out_value,
				  struct sbi_trap_info *out_trap,
				  const struct fdt_match *match)
{
	int ret = 0;

	switch (funcid) {
	case SBI_EXT_ANDES_GET_MCACHE_CTL_STATUS:
		*out_value = csr_read(CSR_MCACHE_CTL);
		break;
	case SBI_EXT_ANDES_GET_MMISC_CTL_STATUS:
		*out_value = csr_read(CSR_MMISC_CTL);
		break;
	case SBI_EXT_ANDES_SET_MCACHE_CTL:
		mcall_set_mcache_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_SET_MMISC_CTL:
		mcall_set_mmisc_ctl(regs->a0);
		break;
	case SBI_EXT_ANDES_DCACHE_WBINVAL_ALL:
		mcall_dcache_wbinval_all();
		break;
	case SBI_EXT_ANDES_DCACHE_OP:
		mcall_dcache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_ICACHE_OP:
	case SBI_EXT_ANDES_L1CACHE_I_PREFETCH:
	case SBI_EXT_ANDES_L1CACHE_D_PREFETCH:
	case SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE:
	case SBI_EXT_ANDES_WRITE_AROUND:
	case SBI_EXT_ANDES_SET_PFM:
		sbi_panic("%s(): deprecated cache SBI call (funcid: %#lx)\n",
			  __func__, funcid);
		break;
	case SBI_EXT_ANDES_TRIGGER:
		*out_value = mcall_set_trigger(regs->a0, regs->a1, 0, 0, regs->a2);
		break;
	case SBI_EXT_ANDES_READ_POWERBRAKE:
		*out_value = csr_read(CSR_MPFT_CTL);
		break;
	case SBI_EXT_ANDES_WRITE_POWERBRAKE:
		csr_write(CSR_MPFT_CTL, regs->a0);
		break;
	case SBI_EXT_ANDES_SET_PMA:
		ret = mcall_set_pma(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_ANDES_FREE_PMA:
		ret = mcall_free_pma(regs->a0);
		break;
	case SBI_EXT_ANDES_PROBE_PMA:
		*out_value = mcall_probe_pma();
		break;
	case SBI_EXT_ANDES_HPM:
		*out_value = andes_hpm();
		break;
	default:
		sbi_panic("%s(): funcid: %#lx is not supported\n", __func__, funcid);
		break;
	}

	return ret;
}
