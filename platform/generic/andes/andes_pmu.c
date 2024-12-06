// SPDX-License-Identifier: BSD-2-Clause
/*
 * andes_pmu.c - Andes PMU device callbacks and platform overrides
 *
 * Copyright (C) 2023 Andes Technology Corporation
 */

#include <andes/andes.h>
#include <andes/andes_pmu.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_pmu.h>
#include <sbi_utils/cache/cache.h>
#include <libfdt.h>

int andes_fw_event_validate_encoding(uint32_t hartid, uint64_t event_data)
{
	/* 25-series doesn't support L2C HPM */
	if (is_andes(25))
		return SBI_EINVAL;

	if (!cache_hpm_idle()) {
		/*
		 * We only use HPM counter 0, let the match fail if the
		 * counter is being used.
		 */
		return SBI_EINVAL;
	}

	return (event_data < ANDES_CUSTOM_FW_EVENT_MAX)
	       ? SBI_PMU_EVENT_TYPE_FW : SBI_EINVAL;
}

bool andes_fw_counter_match_encoding(uint32_t hartid,
				     uint32_t counter_index,
				     uint64_t event_data)
{
	/*
	 * Allow any firmware counter (totally 16) to record
	 * custom firmware events
	 */
	return true;
}

uint64_t andes_fw_counter_read_value(uint32_t hartid, uint32_t counter_index)
{
	uint64_t val = 0;
	cache_read_hpm_ctr(&val);
	return val;
}

void andes_fw_counter_write_value(uint32_t hartid,
				  uint32_t counter_index,
				  uint64_t value)
{
	cache_write_hpm_ctr(value);
}

int andes_fw_counter_start(uint32_t hartid, uint32_t counter_index,
			   uint64_t event_data)
{
	return cache_start_hpm(event_data);
}

int andes_fw_counter_stop(uint32_t hartid, uint32_t counter_index)
{
	return cache_stop_hpm();
}

static void andes_hw_counter_enable_irq(uint32_t ctr_idx)
{
	unsigned long mip_val;

	if (ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return;

	mip_val = csr_read(CSR_MIP);
	if (!(mip_val & MIP_PMOVI))
		csr_clear(CSR_MCOUNTEROVF, BIT(ctr_idx));

	csr_set(CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static void andes_hw_counter_disable_irq(uint32_t ctr_idx)
{
	csr_clear(CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static void andes_hw_counter_filter_mode(unsigned long flags, int ctr_idx)
{
	if (flags & SBI_PMU_CFG_FLAG_SET_UINH)
		csr_set(CSR_MCOUNTERMASK_U, BIT(ctr_idx));
	else
		csr_clear(CSR_MCOUNTERMASK_U, BIT(ctr_idx));

	if (flags & SBI_PMU_CFG_FLAG_SET_SINH)
		csr_set(CSR_MCOUNTERMASK_S, BIT(ctr_idx));
	else
		csr_clear(CSR_MCOUNTERMASK_S, BIT(ctr_idx));
}

static struct sbi_pmu_device andes_pmu = {
	.name = "andes_pmu",
	.fw_event_validate_encoding = andes_fw_event_validate_encoding,
	.fw_counter_match_encoding = andes_fw_counter_match_encoding,
	.fw_counter_read_value = andes_fw_counter_read_value,
	.fw_counter_write_value = andes_fw_counter_write_value,
	.fw_counter_start = andes_fw_counter_start,
	.fw_counter_stop = andes_fw_counter_stop,
	.hw_counter_enable_irq  = andes_hw_counter_enable_irq,
	.hw_counter_disable_irq = andes_hw_counter_disable_irq,
	/*
	 * We set delegation of supervisor local interrupts via
	 * 18th bit on mslideleg instead of mideleg, so leave
	 * hw_counter_irq_bit() callback unimplemented.
	 */
	.hw_counter_irq_bit     = NULL,
	.hw_counter_filter_mode = andes_hw_counter_filter_mode
};

int andes_pmu_extensions_init(const struct fdt_match *match,
			      struct sbi_hart_features *hfeatures)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	if (!has_andes_pmu())
		return 0;

	/*
	 * Don't expect both Andes PMU and standard Sscofpmf/Smcntrpmf,
	 * are supported as they serve the same purpose.
	 */
	if (sbi_hart_has_extension(scratch, SBI_HART_EXT_SSCOFPMF) ||
	    sbi_hart_has_extension(scratch, SBI_HART_EXT_SMCNTRPMF))
		return SBI_EINVAL;
	sbi_hart_update_extension(scratch, SBI_HART_EXT_XANDESPMU, true);

	/* Inhibit all HPM counters in M-mode */
	csr_write(CSR_MCOUNTERMASK_M, 0xfffffffd);
	/* Delegate counter overflow interrupt to S-mode */
	csr_write(CSR_MSLIDELEG, MIP_PMOVI);

	return 0;
}

int andes_pmu_init(const struct fdt_match *match)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();

	if (sbi_hart_has_extension(scratch, SBI_HART_EXT_XANDESPMU))
		sbi_pmu_set_device(&andes_pmu);

	return 0;
}
