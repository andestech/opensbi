/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <platform_override.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/sys/atcsmu.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_init.h>
#include <andes/ae350.h>
#include <andes/andesv5.h>
#include <andes/cache.h>
#include <andes/pma.h>
#include <andes/trigger.h>

static struct smu_data smu = { 0 };
extern void __ae350_enable_coherency_warmboot(void);
extern void __ae350_disable_coherency(void);

static __always_inline bool is_andes25(void)
{
	ulong marchid = csr_read(CSR_MARCHID);
	return !!(EXTRACT_FIELD(marchid, CSR_MARCHID_MICROID) == 0xa25);
}

static int ae350_hart_start(u32 hartid, ulong saddr)
{
	/* Don't send wakeup command at boot-time */
	if (!sbi_init_count(hartid) || (is_andes25() && hartid == 0))
		return sbi_ipi_raw_send(hartid);

	/* Write wakeup command to the sleep hart */
	smu_set_command(&smu, WAKEUP_CMD, hartid);

	return 0;
}

static int ae350_hart_stop(void)
{
	int rc;
	u32 hartid = current_hartid();

	/**
	 * For Andes AX25MP, the hart0 shares power domain with
	 * L2-cache, instead of turning it off, it should fall
	 * through and jump to warmboot_addr.
	 */
	if (is_andes25() && hartid == 0)
		return SBI_ENOTSUPP;

	if (!smu_support_sleep_mode(&smu, DEEPSLEEP_MODE, hartid))
		return SBI_ENOTSUPP;

	/**
	 * disable all events, the current hart will be
	 * woken up from reset vector when other hart
	 * writes its PCS (power control slot) control
	 * register
	 */
	smu_set_wakeup_events(&smu, 0x0, hartid);
	smu_set_command(&smu, DEEP_SLEEP_CMD, hartid);

	rc = smu_set_reset_vector(&smu, (ulong)__ae350_enable_coherency_warmboot,
			       hartid);
	if (rc)
		goto fail;

	__ae350_disable_coherency();

	wfi();

fail:
	/* It should never reach here */
	sbi_hart_hang();
	return 0;
}

static const struct sbi_hsm_device andes_smu = {
	.name	      = "andes_smu",
	.hart_start   = ae350_hart_start,
	.hart_stop    = ae350_hart_stop,
};

static void ae350_hsm_device_init(void)
{
	int rc;
	void *fdt;

	fdt = fdt_get_address();

	rc = fdt_parse_compat_addr(fdt, (uint64_t *)&smu.addr,
				   "andestech,atcsmu");

	if (!rc) {
		sbi_hsm_set_device(&andes_smu);
	}
}

static inline unsigned long mcall_set_pfm(void)
{
	csr_clear(CSR_SLIP, CSR_SLIP_PMOVI_MASK);
	csr_set(CSR_MIE, CSR_MIE_PMOVI_MASK);
	return 0;
}

static int ae350_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (!cold_boot)
		return 0;

	pma_init();
	ae350_hsm_device_init();
	trigger_init();

	return 0;
}

static const struct fdt_match andes_ae350_match[] = {
	{ .compatible = "andestech,ae350" },
	{ },
};

static int ae350_vendor_ext_provider(long extid, long funcid,
					      const struct sbi_trap_regs *regs,
					      unsigned long *out_value,
					      struct sbi_trap_info *out_trap,
					      const struct fdt_match *match)
{
	int ret = 0;

	switch (funcid) {
#ifdef CONFIG_ANDES_CACHE
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

	case SBI_EXT_ANDES_ICACHE_OP:
	case SBI_EXT_ANDES_DCACHE_OP:
	case SBI_EXT_ANDES_L1CACHE_I_PREFETCH:
	case SBI_EXT_ANDES_L1CACHE_D_PREFETCH:
	case SBI_EXT_ANDES_NON_BLOCKING_LOAD_STORE:
	case SBI_EXT_ANDES_WRITE_AROUND:
		sbi_panic("%s(): deprecated cache SBI call (funcid: %#lx)\n",
				__func__, funcid);
		break;
#endif
#ifdef CONFIG_ANDES_TRIGGER
	case SBI_EXT_ANDES_TRIGGER:
		*out_value = mcall_set_trigger(regs->a0, regs->a1, 0, 0, regs->a2);
		break;
#endif
	case SBI_EXT_ANDES_SET_PFM:
		ret = mcall_set_pfm();
		break;
	case SBI_EXT_ANDES_READ_POWERBRAKE:
		*out_value = csr_read(CSR_MPFT_CTL);
		break;
	case SBI_EXT_ANDES_WRITE_POWERBRAKE:
		csr_write(CSR_MPFT_CTL, regs->a0);
		break;
#ifdef CONFIG_ANDES_PMA
	case SBI_EXT_ANDES_SET_PMA:
		ret = mcall_set_pma(regs->a0, regs->a1, regs->a2);
		break;
	case SBI_EXT_ANDES_FREE_PMA:
		ret = mcall_free_pma(regs->a0);
		break;
	case SBI_EXT_ANDES_PROBE_PMA:
		*out_value = mcall_probe_pma();
		break;
#endif
	case SBI_EXT_ANDES_HPM:
		*out_value = andes_hpm();
		break;
	default:
		sbi_panic("%s(): funcid: %#lx is not supported\n", __func__, funcid);
		break;
	}

	return ret;
}

const struct platform_override andes_ae350 = {
	.match_table = andes_ae350_match,
	.final_init  = ae350_final_init,
	.vendor_ext_provider = ae350_vendor_ext_provider,
};
