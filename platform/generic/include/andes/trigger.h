/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 */

#ifndef _AE350_TRIGGER_H
#define _AE350_TRIGGER_H

#include <sbi/sbi_types.h>
#include <sbi/riscv_asm.h>

#define TRIGGER_MAX	16

#define TRIGGER_TYPE_NOT_EXIST 		0UL
#define TRIGGER_TYPE_SIFIVE		1UL
#define TRIGGER_TYPE_MCONTROL		2UL
#define TRIGGER_TYPE_ICOUNT		3UL
#define TRIGGER_TYPE_ITRIGGER		4UL
#define TRIGGER_TYPE_ETRIGGER		5UL
#define TRIGGER_TYPE_NOT_AVAILABLE	15UL

#if __riscv_xlen == 64
#define RISCV_MXLEN	64
#else
#define RISCV_MXLEN	32
#endif

#define TDATA1_OFFSET_TYPE	(RISCV_MXLEN - 4)
#define TDATA1_OFFSET_DMOEE	(RISCV_MXLEN - 5)

#define MCONTROL_OFFSET_MASKMAX	(RISCV_MXLEN - 11)
#define MCONTROL_OFFSET_HIT	20
#define MCONTROL_OFFSET_SELECT	19
#define MCONTROL_OFFSET_TIMING	18
#define MCONTROL_OFFSET_ACTION	12
#define MCONTROL_OFFSET_CHAIN	11
#define MCONTROL_OFFSET_MATCH	7
#define MCONTROL_OFFSET_M	6
#define MCONTROL_OFFSET_S	4
#define MCONTROL_OFFSET_U	3
#define MCONTROL_OFFSET_EXECUTE	2
#define MCONTROL_OFFSET_STORE	1
#define MCONTROL_OFFSET_LOAD	0

#define ICOUNT_OFFSET_HIT	24
#define ICOUNT_OFFSET_COUNT	10
#define ICOUNT_OFFSET_M		9
#define ICOUNT_OFFSET_S		7
#define ICOUNT_OFFSET_U		6
#define ICOUNT_OFFSET_ACTION	0

#define ITRIGGER_OFFSET_HIT	(RISCV_MXLEN - 6)
#define ITRIGGER_OFFSET_M	9
#define ITRIGGER_OFFSET_S	7
#define ITRIGGER_OFFSET_U	6
#define ITRIGGER_OFFSET_ACTION	0

#define ETRIGGER_OFFSET_HIT	(RISCV_MXLEN - 6)
#define ETRIGGER_OFFSET_M	9
#define ETRIGGER_OFFSET_S	7
#define ETRIGGER_OFFSET_U	6
#define ETRIGGER_OFFSET_ACTION	0

#if __riscv_xlen == 64
#define TEXTRA_OFFSET_MVALUE	51
#define TEXTRA_OFFSET_MSELECT	50
#else
#define TEXTRA_OFFSET_MVALUE	26
#define TEXTRA_OFFSET_MSELECT	25
#endif
#define TEXTRA_OFFSET_SVALUE	2
#define TEXTRA_OFFSET_SSELECT	0
#define TEXTRA_SELECT_IGNORE	0
#define TEXTRA_SELECT_CONTEXT	1
#define TEXTRA_SELECT_ASID	2

#define TRIGGER_SUPPORT(n)	(n < total_triggers)
#define TRIGGER_SUPPORT_TYPE(type) ({		\
  uintptr_t __tmp;				\
  uintptr_t __tinfo = csr_read(CSR_TINFO);		\
  __tmp = (__tinfo & (1 << type));		\
  __tmp; })					\

void trigger_init(void);
unsigned long mcall_set_trigger(long type, uintptr_t data, unsigned int m, unsigned int s, unsigned int u);

struct trigger_module {
  int type;
  int used;
};

#endif /* _AE350_TRIGGER_H */
