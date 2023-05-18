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

static const struct hw_evt_select andes_hw_evt_selects[] = {
	/* Hardware general events (Type #0) */
	{
		/* perf: cache-references (eidx: 0x3) */
		.eidx = SBI_PMU_HW_CACHE_REFERENCES,
		.select = DCACHE_ACCESS
	},
	{
		/* perf: cache-misses (eidx: 0x4) */
		.eidx = SBI_PMU_HW_CACHE_MISSES,
		.select = DCACHE_MISS
	},
	/* Hardware cache events (Type #1) */
	{
		/* perf: L1-dcache-loads (eidx: 0x10000) */
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = DCACHE_LOAD_ACCESS
	},
	{
		/* perf: L1-dcache-loads-misses (eidx: 0x10001) */
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = DCACHE_LOAD_MISS
	},
	{
		/* perf: L1-dcache-stores (eidx: 0x10002) */
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_WRITE << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = DCACHE_STORE_ACCESS
	},
	{
		/* perf: L1-dcache-store-misses (eidx: 0x10003) */
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_WRITE << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = DCACHE_STORE_MISS
	},
	{
		/* perf: L1-icache-load (eidx: 0x10008) */
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1I << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = ICACHE_ACCESS
	},
	{
		/* perf: L1-icache-load-misses (eidx: 0x10009) */
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1I << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = ICACHE_MISS
	},
	{ /* sentinel */ }
};

static const struct hw_evt_counter andes_hw_evt_counters[] = {
	{
		/* perf: cache-references (eidx: 0x3) */
		.eidx_start = SBI_PMU_HW_CACHE_REFERENCES,
		/* perf: cache-misses (eidx: 0x4) */
		.eidx_end = SBI_PMU_HW_CACHE_MISSES,
		.ctr_map = ANDES_COUNTERMAP,
	},
	{
		/* perf: L1-dcache-loads (eidx: 0x10000) */
		.eidx_start = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		/* perf: L1-dcache-store-misses (eidx: 0x10003) */
		.eidx_end = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_WRITE << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.ctr_map = ANDES_COUNTERMAP,
	},
	{
		/* perf: L1-icache-load (eidx: 0x10008) */
		.eidx_start = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1I << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		/* perf: L1-icache-load-misses (eidx: 0x10009) */
		.eidx_end = (SBI_PMU_EVENT_TYPE_HW_CACHE << SBI_PMU_EVENT_IDX_TYPE_OFFSET |
				SBI_PMU_HW_CACHE_L1I << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.ctr_map = ANDES_COUNTERMAP,
	},
	{ /* sentinel */ }
};

static const struct raw_evt_counter andes_raw_evt_counters[] = {
	{
		.select = INT_LOAD_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = INT_STORE_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = ATOMIC_MEM_OP,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = SYS_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = INT_COMPUTE_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = TAKEN_CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = JAL_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = JALR_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = RET_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CONTROL_TRANS_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = EX9_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = INT_MUL_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = INT_DIV_REMAINDER_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = FLOAT_LOAD_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = FLOAT_STORE_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = FLOAT_ADD_SUB_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = FLOAT_MUL_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = FLOAT_FUSED_MULADD_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = FLOAT_DIV_SQUARE_ROOT_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = OTHER_FLOAT_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = INT_MUL_AND_SUB_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = RETIRED_OP,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = ILM_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DLM_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = ICACHE_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = ICACHE_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_LOAD_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_LOAD_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_STORE_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_STORE_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = DCACHE_WB,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_ICACHE_FILL,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_DCACHE_FILL,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = UNCACHED_IFETCH_FROM_BUS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = UNCACHED_LOAD_FROM_BUS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_UNCACHED_IFETCH,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_UNCACHED_LOAD,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MAIN_ITLB_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MAIN_ITLB_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MAIN_DTLB_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MAIN_DTLB_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_ITLB_FILL,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = PIPE_STALL_CYCLE_DTLB_MISS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = HW_PREFETCH_BUS_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_OPERAND_READY,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MISPREDICT_CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MISPREDICT_TAKE_CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = MISPREDICT_TARGET_RET_INST,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{
		.select = REPLAY_LAS_SAS,
		.select_mask = RAW_EVENT_MASK,
		.ctr_map = ANDES_COUNTERMAP
	},
	{ /* sentinel */ }
};

static int ae350_fdt_add_pmu(void)
{
	return fdt_add_pmu(andes_hw_evt_selects, andes_hw_evt_counters,
				andes_raw_evt_counters);
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
