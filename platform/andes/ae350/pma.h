/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *	Nick Hu <nickhu@andestech.com>
 *	Nylon Chen <nylon7@andestech.com>
 */


#ifndef _RISCV_PMA_H
#define _RISCV_PMA_H

#define PMA_MMSC_CFG (1 << 30)
#define PMA_NUM 16
#define PMA_NAPOT 0x3
#define PMA_NOCACHE_BUFFER (0x3 << 2)

#define PMAADDR_0	0xBD0
#define PMAADDR_1	0xBD1
#define PMAADDR_2	0xBD2
#define PMAADDR_3	0xBD3
#define PMAADDR_4	0xBD4
#define PMAADDR_5	0xBD5
#define PMAADDR_6	0xBD6
#define PMAADDR_7	0xBD7
#define PMAADDR_8	0xBD8
#define PMAADDR_9	0xBD9
#define PMAADDR_10	0xBDA
#define PMAADDR_11	0xBDB
#define PMAADDR_12	0xBDC
#define PMAADDR_13	0xBDD
#define PMAADDR_14	0xBDE
#define PMAADDR_15	0xBDF

#define PMACFG_0	0xBC0
#define PMACFG_1	0xBC1
#define PMACFG_2	0xBC2
#define PMACFG_3	0xBC3

#ifndef __ASSEMBLER__

extern unsigned long pma_used_table[PMA_NUM];

void write_pmaaddr(int i, unsigned long val);

unsigned long read_pmacfg(int i);

void write_pmacfg(int i, unsigned long val);

void mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size, unsigned long entry_id);
void mcall_free_pma(unsigned long va);

void init_pma(void);

#endif
#endif
