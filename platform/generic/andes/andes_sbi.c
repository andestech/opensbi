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
#include <andes/remoteproc.h>
#include <andes/andes_pma.h>
#include <andes/trigger.h>
#include <sbi_utils/cache/cache.h>

enum sbi_ext_andes_fid {
	SBI_EXT_ANDES_FID0 = 0, /* Reserved for future use */
	SBI_EXT_ANDES_IOCP_SW_WORKAROUND,
	SBI_EXT_ANDES_TRIGGER_SET = ANDES_SBI_INTERNAL_FID_START,
	SBI_EXT_ANDES_POWERBRAKE_READ,
	SBI_EXT_ANDES_POWERBRAKE_WRITE,
	SBI_EXT_ANDES_PMA_SET,
	SBI_EXT_ANDES_PMA_FREE,
	SBI_EXT_ANDES_PMA_PROBE,
	SBI_EXT_ANDES_DCACHE_EN,
	SBI_EXT_ANDES_REMOTEPROC_EN,
	SBI_EXT_ANDES_REMOTEPROC_SEND_IPI,
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

static __always_inline void mcall_dcache_op(unsigned int enable)
{
	int rc;

	if (enable) {
		csr_set(CSR_MCACHE_CTL, MCACHE_CTL_DC_EN);
		rc = cache_enable(); /* L2C enable */
		if (rc && (rc != SBI_ENODEV))
			sbi_printf("%s: WARN: L2-cache enable failed\n", __func__);
	} else {
		sbi_printf(
			"%s: WARN: The use of 'disable d-cache' is deprecated.\n",
			__func__);
	}
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
	/*
	 * Before accessing the mpft_ctl CSR, we need to check if the
	 * PowerBrake feature is supported or not.
	 */
	case SBI_EXT_ANDES_POWERBRAKE_READ:
		if (andes_powerbrake()) {
			out->value = csr_read(CSR_MPFT_CTL);
		} else {
			out->value = 0;
		}
		break;
	case SBI_EXT_ANDES_POWERBRAKE_WRITE:
		if (andes_powerbrake()) {
			csr_write(CSR_MPFT_CTL, regs->a0);
		}
		break;
	case SBI_EXT_ANDES_PMA_SET:
		ret = mcall_set_pma(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_ANDES_PMA_FREE:
		ret = mcall_free_pma(regs->a0);
		break;
	case SBI_EXT_ANDES_PMA_PROBE:
		out->value = mcall_probe_pma();
		break;
	case SBI_EXT_ANDES_DCACHE_EN:
		mcall_dcache_op(regs->a0);
		break;
	case SBI_EXT_ANDES_REMOTEPROC_EN:
		remoteproc_ipi_enable(regs->a0);
		break;
	case SBI_EXT_ANDES_REMOTEPROC_SEND_IPI:
		sbi_ipi_raw_send(regs->a0);
		break;

	default:
		sbi_panic("%s(): funcid: %#lx is not supported\n", __func__, funcid);
		break;
	}

	return ret;
}
