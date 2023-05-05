/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * Authors:
 *  Locus Chen <locus84@andestech.com>
 */

#ifndef __PMU_H__
#define __PMU_H__

#include <sbi/sbi_ecall_interface.h>
#include <sbi_utils/fdt/fdt_fixup.h>

#define	MCOUNTER3	(1 << 3)
#define	MCOUNTER4	(1 << 4)
#define	MCOUNTER5	(1 << 5)
#define	MCOUNTER6	(1 << 6)
#define	ANDES_COUNTERMAP	(MCOUNTER3 | MCOUNTER4 | \
				MCOUNTER5 | MCOUNTER6)

#define	RAW_EVENT_MASK	~0ULL

/* Event code for instruction commit events */
#define	RISCV_CYCLE_COUNT	0x10
#define	RISCV_INSTRET	0x20
#define	INT_LOAD_INST	0x30
#define	INT_STORE_INST	0x40
#define	ATOMIC_MEM_OP	0x50
#define	SYS_INST	0x60
#define	INT_COMPUTE_INST	0x70
#define	CONDITION_BR	0x80
#define	TAKEN_CONDITION_BR	0x90
#define	JAL_INST	0xA0
#define	JALR_INST	0xB0
#define	RET_INST	0xC0
#define	CONTROL_TRANS_INST	0xD0
#define	EX9_INST	0xE0
#define	INT_MUL_INST	0xF0
#define	INT_DIV_REMAINDER_INST	0x100
#define	FLOAT_LOAD_INST	0x110
#define	FLOAT_STORE_INST	0x120
#define	FLOAT_ADD_SUB_INST	0x130
#define	FLOAT_MUL_INST	0x140
#define	FLOAT_FUSED_MULADD_INST	0x150
#define	FLOAT_DIV_SQUARE_ROOT_INST	0x160
#define	OTHER_FLOAT_INST	0x170
#define	INT_MUL_AND_SUB_INST	0x180
#define	RETIRED_OP	0x190

/* Event code for memory system events */
#define	ILM_ACCESS	0x01
#define	DLM_ACCESS	0x11
#define	ICACHE_ACCESS	0x21
#define	ICACHE_MISS	0x31
#define	DCACHE_ACCESS	0x41
#define	DCACHE_MISS	0x51
#define	DCACHE_LOAD_ACCESS	0x61
#define	DCACHE_LOAD_MISS	0x71
#define	DCACHE_STORE_ACCESS	0x81
#define	DCACHE_STORE_MISS	0x91
#define	DCACHE_WB	0xA1
#define	CYCLE_WAIT_ICACHE_FILL	0xB1
#define	CYCLE_WAIT_DCACHE_FILL	0xC1
#define	UNCACHED_IFETCH_FROM_BUS	0xD1
#define	UNCACHED_LOAD_FROM_BUS	0xE1
#define	CYCLE_WAIT_UNCACHED_IFETCH	0xF1
#define	CYCLE_WAIT_UNCACHED_LOAD	0x101
#define	MAIN_ITLB_ACCESS	0x111
#define	MAIN_ITLB_MISS	0x121
#define	MAIN_DTLB_ACCESS	0x131
#define	MAIN_DTLB_MISS	0x141
#define	CYCLE_WAIT_ITLB_FILL	0x151
#define	PIPE_STALL_CYCLE_DTLB_MISS	0x161
#define	HW_PREFETCH_BUS_ACCESS	0x171
#define	CYCLE_WAIT_OPERAND_READY	0x181

/* Event code for microarchitecture events */
#define	MISPREDICT_CONDITION_BR	0x02
#define	MISPREDICT_TAKE_CONDITION_BR	0x12
#define	MISPREDICT_TARGET_RET_INST	0x22

/* LAS: load after store, SAS: store after store */
#define	REPLAY_LAS_SAS	0x32

int ae350_fdt_add_pmu(void);

#endif /* PMU_H */
