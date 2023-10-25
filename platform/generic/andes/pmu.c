/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *  Locus Chen <locus84@andestech.com>
 *  Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_types.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <andes/andesv5.h>
#include <andes/l2c_hpm.h>
#include <andes/pmu.h>
#include <andes/andes_hpm.h>

static int ae350_fdt_add_pmu(void)
{
	switch (csr_read(CSR_MARCHID) & 0xff)
	{
	case 0x25:
		return fdt_add_pmu_25();
	case 0x27:
		return fdt_add_pmu_27();
	case 0x45:
		return fdt_add_pmu_45();
	case 0x65:
		return fdt_add_pmu_65();
	default:
		return 0;
	}
}

static void ae350_hw_counter_enable_irq(uint32_t ctr_idx)
{
	unsigned long mip_val;

	if (ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return;

	mip_val = csr_read(CSR_MIP);
	if (!(mip_val & MIP_PMOVI))
		csr_clear(CSR_MCOUNTEROVF, BIT(ctr_idx));

	csr_set(CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static void ae350_hw_counter_disable_irq(uint32_t ctr_idx)
{
	csr_clear(CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static void ae350_hw_counter_filter_mode(unsigned long flags, int ctr_idx)
{
	if (!flags) {
		csr_write(CSR_MCOUNTERMASK_S, 0);
		csr_write(CSR_MCOUNTERMASK_U, 0);
	}
	if (flags & SBI_PMU_CFG_FLAG_SET_UINH) {
		csr_clear(CSR_MCOUNTERMASK_S, BIT(ctr_idx));
		csr_set(CSR_MCOUNTERMASK_U, BIT(ctr_idx));
	}
	if (flags & SBI_PMU_CFG_FLAG_SET_SINH) {
		csr_set(CSR_MCOUNTERMASK_S, BIT(ctr_idx));
		csr_clear(CSR_MCOUNTERMASK_U, BIT(ctr_idx));
	}
}

static struct sbi_pmu_device ae350_pmu = {
	.name	= "andes_pmu",
	.fw_event_validate_code = ae350_fw_event_validate_code,
	.fw_counter_match_code   = ae350_fw_counter_match_code,
	.fw_counter_read_value = ae350_fw_counter_read_value,
	.fw_counter_start = ae350_fw_counter_start,
	.fw_counter_stop = ae350_fw_counter_stop,
	/*
	 * We will add hw_counter_* callbacks if andes_hpm is supported,
	 * if Sscofpmf is supported, these callbacks should not exist.
	 */
	.hw_counter_enable_irq = NULL,
	.hw_counter_disable_irq = NULL,
	.hw_counter_irq_bit = NULL,
	.hw_counter_filter_mode = NULL
};

int ae350_pmu_init(bool cold_boot)
{
	int rc;
	/* Setup Andes specific HPM CSRs for each hart */
	if (andes_hpm()) {
		/* Machine counter write enable */
		csr_write(CSR_MCOUNTERWEN, 0xfffffffd);
		/* Supervisor local interrupt enable */
		csr_write(CSR_SLIE, MIP_PMOVI);
		/* disable machine counter in M-mode */
		csr_write(CSR_MCOUNTERMASK_M, 0xfffffffd);
		/* delegate S-mode local interrupt to S-mode */
		csr_write(CSR_MSLIDELEG, MIP_PMOVI);

		/* Make sure Sscofpmf is disabled */
		sbi_hart_update_extension(sbi_scratch_thishart_ptr(),
				SBI_HART_EXT_SSCOFPMF, false);
	}

	if (!cold_boot)
		return 0;

	if (andes_hpm()) {
		ae350_pmu.hw_counter_enable_irq = ae350_hw_counter_enable_irq;
		ae350_pmu.hw_counter_disable_irq = ae350_hw_counter_disable_irq;
		/*
		 * The irq bit (18) should be set on mslideleg instead of mideleg,
		 * hence hw_counter_irq_bit is unimplemented.
		 */
		ae350_pmu.hw_counter_irq_bit = NULL;
		ae350_pmu.hw_counter_filter_mode = ae350_hw_counter_filter_mode;
	}

	sbi_pmu_set_device(&ae350_pmu);

	rc = ae350_fdt_add_pmu();
	if (rc)
		return rc;

	return 0;
}
