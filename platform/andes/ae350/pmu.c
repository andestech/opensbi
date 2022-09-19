/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *  Locus Chen <locus84@andestech.com>
 */

#include <sbi/sbi_pmu.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include "cache.h"
#include "platform.h"
#include "pmu.h"

#define	COUNTERMAP	(MCOUNTER3 | MCOUNTER4 | \
				MCOUNTER5 | MCOUNTER6)

int hw_l2c_counter_start(int idx, uint64_t ival, unsigned long flags)
{
	if (flags == SBI_PMU_ANDES_L2C_EVENT &&
		!(flags & ~SBI_PMU_ANDES_L2C_EVENT)) {
		l2c_write_counter(idx, ival);
		return 0;
	}
	return SBI_EINVAL;
}

int hw_l2c_counter_stop(int idx, unsigned long flag)
{
	if (flag == SBI_PMU_ANDES_L2C_EVENT &&
		!(flag & ~SBI_PMU_ANDES_L2C_EVENT)) {
		l2c_pmu_disable_counter(idx);
		return 0;
	}
	return SBI_EINVAL;
}

#define	MAX_EVNET_NUM	64

struct andes_pmu_hw_event_select {
	uint32_t eidx;
	uint64_t select;
};

struct andes_pmu_hw_event_select andes_pmu_evt_select[MAX_EVNET_NUM] = {
	{
		.eidx = SBI_PMU_HW_CACHE_REFERENCES,
		.select = DCACHE_ACCESS
	},
	{
		.eidx = SBI_PMU_HW_CACHE_MISSES,
		.select = DCACHE_MISS
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = DCACHE_LOAD_ACCESS
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = DCACHE_LOAD_MISS
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_WRITE << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = DCACHE_STORE_ACCESS
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_WRITE << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = DCACHE_STORE_MISS
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_PREFETCH << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = NOT_SUPPORT
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1D << 3 |
				SBI_PMU_HW_CACHE_OP_PREFETCH << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = NOT_SUPPORT
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1I << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_ACCESS << 0),
		.select = ICACHE_ACCESS
	},
	{
		.eidx = (SBI_PMU_EVENT_TYPE_HW_CACHE << 16 |
				SBI_PMU_HW_CACHE_L1I << 3 |
				SBI_PMU_HW_CACHE_OP_READ << 1 |
				SBI_PMU_HW_CACHE_RESULT_MISS << 0),
		.select = ICACHE_MISS
	}
};

struct andes_pmu_raw_event_select {
	uint64_t select;
	uint64_t select_mask;
	uint32_t counter_bit;
};

static struct andes_pmu_raw_event_select andes_pmu_raw_evt[MAX_EVNET_NUM] = {
	{
		.select = INT_LOAD_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = INT_STORE_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = ATOMIC_MEM_OP,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = SYS_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = INT_COMPUTE_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = TAKEN_CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = JAL_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = JALR_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = RET_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CONTROL_TRANS_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = EX9_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = INT_MUL_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = INT_DIV_REMAINDER_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = FLOAT_LOAD_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = FLOAT_STORE_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = FLOAT_ADD_SUB_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = FLOAT_MUL_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = FLOAT_FUSED_MULADD_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = FLOAT_DIV_SQUARE_ROOT_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = OTHER_FLOAT_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = INT_MUL_AND_SUB_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = RETIRED_OP,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = ICACHE_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = ICACHE_MISS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_MISS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_LOAD_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_LOAD_MISS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_STORE_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_STORE_MISS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = DCACHE_WB,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_ICACHE_FILL,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_DCACHE_FILL,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = UNCACHED_IFETCH_FROM_BUS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = UNCACHED_LOAD_FROM_BUS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_UNCACHED_IFETCH,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_UNCACHED_LOAD,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = MAIN_ITLB_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = MAIN_ITLB_MISS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = MAIN_DTLB_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_ITLB_FILL,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = PIPE_STALL_CYCLE_DTLB_MISS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = HW_PREFETCH_BUS_ACCESS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = CYCLE_WAIT_OPERAND_READY,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = MISPREDICT_CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = MISPREDICT_TAKE_CONDITION_BR,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = MISPREDICT_TARGET_RET_INST,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	},
	{
		.select = REPLAY_LAS_SAS,
		.select_mask = RAW_EVENT_MASK,
		.counter_bit = COUNTERMAP
	}
};

/*
 * Add this function to support custom raw events without dts.
 */
void andes_pmu_setup(void)
{
	struct andes_pmu_hw_event_select *event;
	struct andes_pmu_raw_event_select *raw_event;
	int i;
	u32 event_idx_start = 0, event_idx_end = 0, ctr_map = COUNTERMAP;

	/* hw event  & hw cache event setup */
	for (i = 0; i < MAX_EVNET_NUM; i++) {
		event = &andes_pmu_evt_select[i];
		event_idx_start = event->eidx;
		event_idx_end = event->eidx;
		if (!event->select) {
			if (!event->eidx)
				break;
			continue;
		}
		sbi_pmu_add_hw_event_counter_map(event_idx_start,
						 event_idx_end,
						 ctr_map);
	}

	/* raw event setup */
	for (i = 0; i < MAX_EVNET_NUM; i++) {
		raw_event = &andes_pmu_raw_evt[i];
		if (!raw_event->select)
			break;
		sbi_pmu_add_raw_event_counter_map(raw_event->select,
						  raw_event->select_mask,
						  raw_event->counter_bit);
	}
}

uint64_t andes_pmu_get_select_value(uint32_t event_idx)
{
	int i;
	struct andes_pmu_hw_event_select *event;

	for (i = 0; i < MAX_EVNET_NUM; i++) {
		event = &andes_pmu_evt_select[i];
		if (event->eidx == event_idx)
			return event->select;
	}

	return 0;
}
