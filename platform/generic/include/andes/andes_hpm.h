/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _ANDES_HPM_H_
#define _ANDES_HPM_H_

#include <sbi_utils/fdt/fdt_helper.h>

#define ANDES_MHPM_MAP		0x78
#define ANDES_RAW_EVENT_MASK	~0ULL

/* Event code for instruction commit events */         /* 25 27 45 65 */
#define ANDES_CYCLES				0x10   /*  Y  Y  Y  Y */
#define ANDES_INSTRET				0x20   /*  Y  Y  Y  Y */
#define ANDES_INT_LOAD_INST			0x30   /*  Y  Y  Y  Y */
#define ANDES_INT_STORE_INST			0x40   /*  Y  Y  Y  Y */
#define ANDES_ATOMIC_INST			0x50   /*  Y  Y  Y  Y */
#define ANDES_SYS_INST				0x60   /*  Y  Y  Y  Y */
#define ANDES_INT_COMPUTE_INST			0x70   /*  Y  Y  Y  Y */
#define ANDES_CONDITION_BR			0x80   /*  Y  Y  Y  Y */
#define ANDES_TAKEN_CONDITION_BR		0x90   /*  Y  Y  Y  Y */
#define ANDES_JAL_INST				0xA0   /*  Y  Y  Y  Y */
#define ANDES_JALR_INST				0xB0   /*  Y  Y  Y  Y */
#define ANDES_RET_INST				0xC0   /*  Y  Y  Y  Y */
#define ANDES_CONTROL_TRANS_INST		0xD0   /*  Y  Y  Y  Y */
#define ANDES_EX9_INST				0xE0   /*  Y  Y  Y  Y */
#define ANDES_INT_MUL_INST			0xF0   /*  Y  Y  Y  Y */
#define ANDES_INT_DIV_REMAINDER_INST		0x100  /*  Y  Y  Y  Y */
#define ANDES_FLOAT_LOAD_INST			0x110  /*  Y  Y  Y  Y */
#define ANDES_FLOAT_STORE_INST			0x120  /*  Y  Y  Y  Y */
#define ANDES_FLOAT_ADD_SUB_INST		0x130  /*  Y  Y  Y  Y */
#define ANDES_FLOAT_MUL_INST			0x140  /*  Y  Y  Y  Y */
#define ANDES_FLOAT_FUSED_MULADD_INST		0x150  /*  Y  Y  Y  Y */
#define ANDES_FLOAT_DIV_SQUARE_ROOT_INST	0x160  /*  Y  Y  Y  Y */
#define ANDES_OTHER_FLOAT_INST			0x170  /*  Y  Y  Y  Y */
#define ANDES_INT_MUL_AND_SUB_INST		0x180  /*  Y  Y  Y  N */
#define ANDES_RETIRED_OP			0x190  /*  N  N  Y  Y */

/* Event code for memory system events */              /* 25 27 45 65 */
#define ANDES_ILM_ACCESS			0x01   /*  Y  Y  Y  N */
#define ANDES_DLM_ACCESS			0x11   /*  Y  Y  Y  N */
#define ANDES_ICACHE_ACCESS			0x21   /*  Y  Y  Y  Y */
#define ANDES_ICACHE_MISS			0x31   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_ACCESS			0x41   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_MISS			0x51   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_LOAD_ACCESS		0x61   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_LOAD_MISS			0x71   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_STORE_ACCESS		0x81   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_STORE_MISS			0x91   /*  Y  Y  Y  Y */
#define ANDES_DCACHE_WB				0xA1   /*  Y  Y  Y  Y */
#define ANDES_CYCLE_WAIT_ICACHE_FILL		0xB1   /*  Y  Y  Y  Y */
#define ANDES_CYCLE_WAIT_DCACHE_FILL		0xC1   /*  Y  Y  Y  Y */
#define ANDES_UNCACHED_IFETCH_FROM_BUS		0xD1   /*  Y  Y  Y  Y */
#define ANDES_UNCACHED_LOAD_FROM_BUS		0xE1   /*  Y  Y  Y  Y */
#define ANDES_CYCLE_WAIT_UNCACHED_IFETCH	0xF1   /*  Y  Y  Y  Y */
#define ANDES_CYCLE_WAIT_UNCACHED_LOAD		0x101  /*  Y  Y  Y  Y */
#define ANDES_MAIN_ITLB_ACCESS			0x111  /*  Y  Y  Y  Y */
#define ANDES_MAIN_ITLB_MISS			0x121  /*  Y  Y  Y  Y */
#define ANDES_MAIN_DTLB_ACCESS			0x131  /*  Y  Y  Y  Y */
#define ANDES_MAIN_DTLB_MISS			0x141  /*  Y  Y  Y  Y */
#define ANDES_CYCLE_WAIT_ITLB_FILL		0x151  /*  Y  Y  Y  Y */
#define ANDES_PIPE_STALL_CYCLE_DTLB_MISS	0x161  /*  Y  Y  Y  Y */
#define ANDES_HW_PREFETCH_BUS_ACCESS		0x171  /*  N  N  Y  Y */
#define ANDES_CYCLE_WAIT_FOR_OPERAND		0x181  /*  N  N  Y  N */
#define ANDES_L1_ITLB_ACCESS			0x191  /*  N  N  N  Y */
#define ANDES_L1_ITLB_MISS			0x1A1  /*  N  N  N  Y */
#define ANDES_L1_DTLB_ACCESS			0x1B1  /*  N  N  N  Y */
#define ANDES_L1_DTLB_MISS			0x1C1  /*  N  N  N  Y */

/* Event code for microarchitecture events */          /* 25 27 45 65 */
#define ANDES_MISPREDICT_CONDITION_BR		0x02   /*  Y  Y  Y  Y */
#define ANDES_MISPREDICT_TAKE_CONDITION_BR	0x12   /*  Y  Y  Y  Y */
#define ANDES_MISPREDICT_TARGET_RET_INST	0x22   /*  Y  Y  Y  Y */
#define ANDES2X_REPLY_LAS_SAS			0x32   /*  Y  Y  N  N */
#define ANDES65_REPLY_LOAD_ORDER_ECC_ERR	0x32   /*  N  N  N  Y */

#ifndef __ASSEMBLER__

int fdt_add_pmu_25(void);
int fdt_add_pmu_27(void);
int fdt_add_pmu_45(void);
int fdt_add_pmu_65(void);

#endif /* __ASSEMBLER__ */

#endif /* _ANDES_HPM_H_ */
