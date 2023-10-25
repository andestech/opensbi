/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 */

#include <andes/andesv5.h>
#include <andes/andes_hpm.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi_utils/fdt/fdt_fixup.h>

static const struct sbi_pmu_event_select_map andes_hw_evt_selects[] = {
	/* Hardware general events (Type #0) */
	{
		/* perf: cycles (eidx: 0x1) */
		.eidx = SBI_PMU_HW_CPU_CYCLES,
		.select = ANDES_CYCLES
	},
	{
		/* perf: instructions (eidx: 0x2) */
		.eidx = SBI_PMU_HW_INSTRUCTIONS,
		.select = ANDES_INSTRET
	},
	{
		/* perf: cache-references (eidx: 0x3) */
		.eidx = SBI_PMU_HW_CACHE_REFERENCES,
		.select = ANDES_DCACHE_ACCESS
	},
	{
		/* perf: cache-misses (eidx: 0x4) */
		.eidx = SBI_PMU_HW_CACHE_MISSES,
		.select = ANDES_DCACHE_MISS
	},
	{
		/* perf: branches (eidx: 0x5) */
		.eidx = SBI_PMU_HW_BRANCH_INSTRUCTIONS,
		.select = ANDES_CONDITION_BR,
	},
	{
		/* perf: branch-misses (eidx: 0x6) */
		.eidx = SBI_PMU_HW_BRANCH_MISSES,
		.select = ANDES_MISPREDICT_CONDITION_BR,
	},
	/* Hardware cache events (Type #1) */
	{
		/* perf: L1-dcache-loads (eidx: 0x10000) */
		.eidx = SBI_PMU_HW_CACHE_EVENT_IDX(L1D, READ, ACCESS),
		.select = ANDES_DCACHE_LOAD_ACCESS
	},
	{
		/* perf: L1-dcache-loads-misses (eidx: 0x10001) */
		.eidx = SBI_PMU_HW_CACHE_EVENT_IDX(L1D, READ, MISS),
		.select = ANDES_DCACHE_LOAD_MISS
	},
	{
		/* perf: L1-dcache-stores (eidx: 0x10002) */
		.eidx = SBI_PMU_HW_CACHE_EVENT_IDX(L1D, WRITE, ACCESS),
		.select = ANDES_DCACHE_STORE_ACCESS
	},
	{
		/* perf: L1-dcache-store-misses (eidx: 0x10003) */
		.eidx = SBI_PMU_HW_CACHE_EVENT_IDX(L1D, WRITE, MISS),
		.select = ANDES_DCACHE_STORE_MISS
	},
	{
		/* perf: L1-icache-load (eidx: 0x10008) */
		.eidx = SBI_PMU_HW_CACHE_EVENT_IDX(L1I, READ, ACCESS),
		.select = ANDES_ICACHE_ACCESS
	},
	{
		/* perf: L1-icache-load-misses (eidx: 0x10009) */
		.eidx = SBI_PMU_HW_CACHE_EVENT_IDX(L1I, READ, MISS),
		.select = ANDES_ICACHE_MISS
	},
	{ /* sentinel */ }
};

static const struct sbi_pmu_event_counter_map andes_hw_evt_counters[] = {
	{
		/* perf: cycles (eidx: 0x1) */
		.eidx_start = SBI_PMU_HW_CPU_CYCLES,
		/* perf: branch-misses (eidx: 0x6) */
		.eidx_end = SBI_PMU_HW_BRANCH_MISSES,
		.ctr_map = ANDES_MHPM_MAP,
	},
	{
		/* perf: L1-dcache-loads (eidx: 0x10000) */
		.eidx_start = SBI_PMU_HW_CACHE_EVENT_IDX(L1D, READ, ACCESS),
		/* perf: L1-dcache-store-misses (eidx: 0x10003) */
		.eidx_end = SBI_PMU_HW_CACHE_EVENT_IDX(L1D, WRITE, MISS),
		.ctr_map = ANDES_MHPM_MAP,
	},
	{
		/* perf: L1-icache-load (eidx: 0x10008) */
		.eidx_start = SBI_PMU_HW_CACHE_EVENT_IDX(L1I, READ, ACCESS),
		/* perf: L1-icache-load-misses (eidx: 0x10009) */
		.eidx_end = SBI_PMU_HW_CACHE_EVENT_IDX(L1I, READ, MISS),
		.ctr_map = ANDES_MHPM_MAP,
	},
	{ /* sentinel */ }
};

static const struct sbi_pmu_raw_event_counter_map andes2x_raw_evt_counters[] = {
	{
		.select = ANDES_CYCLES,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INSTRET,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_LOAD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_STORE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ATOMIC_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_SYS_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_COMPUTE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_TAKEN_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_JAL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_JALR_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_RET_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CONTROL_TRANS_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_EX9_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_MUL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_DIV_REMAINDER_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_LOAD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_STORE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_ADD_SUB_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_MUL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_FUSED_MULADD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_DIV_SQUARE_ROOT_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_OTHER_FLOAT_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_MUL_AND_SUB_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ILM_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DLM_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ICACHE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ICACHE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_LOAD_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_LOAD_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_STORE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_STORE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_WB,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_ICACHE_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_DCACHE_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_UNCACHED_IFETCH_FROM_BUS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_UNCACHED_LOAD_FROM_BUS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_UNCACHED_IFETCH,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_UNCACHED_LOAD,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_ITLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_ITLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_DTLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_ITLB_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_PIPE_STALL_CYCLE_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_TAKE_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_TARGET_RET_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES2X_REPLY_LAS_SAS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{ /* sentinel */ }
};

static const struct sbi_pmu_raw_event_counter_map andes45_raw_evt_counters[] = {
	{
		.select = ANDES_CYCLES,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INSTRET,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_LOAD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_STORE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ATOMIC_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_SYS_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_COMPUTE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_TAKEN_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_JAL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_JALR_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_RET_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CONTROL_TRANS_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_EX9_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_MUL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_DIV_REMAINDER_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_LOAD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_STORE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_ADD_SUB_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_MUL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_FUSED_MULADD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_DIV_SQUARE_ROOT_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_OTHER_FLOAT_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_MUL_AND_SUB_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_RETIRED_OP,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ILM_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DLM_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ICACHE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ICACHE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_LOAD_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_LOAD_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_STORE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_STORE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_WB,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_ICACHE_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_DCACHE_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_UNCACHED_IFETCH_FROM_BUS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_UNCACHED_LOAD_FROM_BUS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_UNCACHED_IFETCH,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_UNCACHED_LOAD,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_ITLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_ITLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_DTLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_ITLB_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_PIPE_STALL_CYCLE_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_HW_PREFETCH_BUS_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_FOR_OPERAND,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_TAKE_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_TARGET_RET_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{ /* sentinel */ }
};

static const struct sbi_pmu_raw_event_counter_map andes65_raw_evt_counters[] = {
	{
		.select = ANDES_CYCLES,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INSTRET,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_LOAD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_STORE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ATOMIC_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_SYS_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_COMPUTE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_TAKEN_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_JAL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_JALR_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_RET_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CONTROL_TRANS_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_EX9_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_MUL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_INT_DIV_REMAINDER_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_LOAD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_STORE_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_ADD_SUB_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_MUL_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_FUSED_MULADD_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_FLOAT_DIV_SQUARE_ROOT_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_OTHER_FLOAT_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_RETIRED_OP,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ICACHE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_ICACHE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_LOAD_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_LOAD_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_STORE_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_STORE_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_DCACHE_WB,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_ICACHE_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_DCACHE_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_UNCACHED_IFETCH_FROM_BUS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_UNCACHED_LOAD_FROM_BUS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_UNCACHED_IFETCH,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_UNCACHED_LOAD,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_ITLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_ITLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_DTLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MAIN_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_CYCLE_WAIT_ITLB_FILL,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_PIPE_STALL_CYCLE_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_HW_PREFETCH_BUS_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_L1_ITLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_L1_ITLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_L1_DTLB_ACCESS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_L1_DTLB_MISS,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_TAKE_CONDITION_BR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES_MISPREDICT_TARGET_RET_INST,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{
		.select = ANDES65_REPLY_LOAD_ORDER_ECC_ERR,
		.select_mask = ANDES_RAW_EVENT_MASK,
		.ctr_map = ANDES_MHPM_MAP
	},
	{ /* sentinel */ }
};

int fdt_add_pmu_25(void)
{
	return fdt_add_pmu(andes_hw_evt_selects,
			   andes_hw_evt_counters,
			   andes2x_raw_evt_counters);
}

int fdt_add_pmu_27(void)
{
	return fdt_add_pmu(andes_hw_evt_selects,
			   andes_hw_evt_counters,
			   andes2x_raw_evt_counters);
}

int fdt_add_pmu_45(void)
{
	return fdt_add_pmu(andes_hw_evt_selects,
			   andes_hw_evt_counters,
			   andes45_raw_evt_counters);
}

int fdt_add_pmu_65(void)
{
	return fdt_add_pmu(andes_hw_evt_selects,
			   andes_hw_evt_counters,
			   andes65_raw_evt_counters);
}
