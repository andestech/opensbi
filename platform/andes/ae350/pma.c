/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 * 	Nick Hu <nickhu@andestech.com>
 *	Nylon Chen <nylon7@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>
#include "platform.h"
#include "pma.h"

unsigned long pma_used_table[PMA_NUM];

void init_pma(void){
  int i;
  for (i = 0; i < PMA_NUM; i++) {
    pma_used_table[i] = 0;
  }
}

void mcall_set_pma(unsigned int pa, unsigned long va, unsigned long size)
{
	int i, power = 0;
	unsigned long size_tmp, shift = 0, pmacfg_val;
	char *pmaxcfg;
	unsigned long mmsc = csr_read(CSR_MMSC_CFG);

	if ((mmsc & PMA_MMSC_CFG) == 0)
		return;

	if ((pa & (size - 1)) !=0) {
		pa = pa & ~(size - 1);
		size = size << 1;
	}

	/* Calculate the NAPOT table for pmaaddr */
	size_tmp = size;
	while (size_tmp != 0x1) {
		size_tmp = size_tmp >> 1;
		power++;
		if (power >3)
			shift = (shift << 1) | 0x1;
	}

	for (i = 0; i < PMA_NUM; i++) {
		if (!pma_used_table[i]) {
			pma_used_table[i] = va;
			pa = pa >> 2;
			pa = pa | shift;
			pa = pa & ~(0x1 << (power - 3));
#if __riscv_xlen == 64
			pmacfg_val = read_pmacfg(i / 8);
			pmaxcfg = (char *)&pmacfg_val;
			pmaxcfg = pmaxcfg + (i % 8);
			*pmaxcfg = 0;
			*pmaxcfg = *pmaxcfg | PMA_NAPOT;
			*pmaxcfg = *pmaxcfg | PMA_NOCACHE_BUFFER;
			write_pmacfg(i / 8, pmacfg_val);
#else
      pmacfg_val = read_pmacfg(i / 4);
      pmaxcfg = (char *)&pmacfg_val;
      pmaxcfg = pmaxcfg + (i % 4);
      *pmaxcfg = 0;
      *pmaxcfg = *pmaxcfg | PMA_NAPOT;
      *pmaxcfg = *pmaxcfg | PMA_NOCACHE_BUFFER;
      write_pmacfg(i / 4, pmacfg_val);
#endif
			write_pmaaddr(i, pa);
			return;
		}
	}
	/* There is no available pma register */
	__asm__("ebreak");


}

void mcall_free_pma(unsigned long va)
{
	int i;
	for (i = 0 ; i < PMA_NUM; i++) {
		if (pma_used_table[i] == va) {
			pma_used_table[i] = 0;
#if __riscv_xlen == 64
			write_pmacfg(i / 8, 0);
#else
			write_pmacfg(i / 4, 0);
#endif
			return;
		}
	}

}

inline void write_pmaaddr(int i, unsigned long val)
{
  if (i == 0)
        csr_write(PMAADDR_0, val);
  else if (i == 1)
        csr_write(PMAADDR_1, val);
  else if (i == 2)
        csr_write(PMAADDR_2, val);
  else if (i == 3)
        csr_write(PMAADDR_3, val);
  else if (i == 4)
        csr_write(PMAADDR_4, val);
  else if (i == 5)
        csr_write(PMAADDR_5, val);
  else if (i == 6)
        csr_write(PMAADDR_6, val);
  else if (i == 7)
        csr_write(PMAADDR_7, val);
  else if (i == 8)
        csr_write(PMAADDR_8, val);
  else if (i == 9)
        csr_write(PMAADDR_9, val);
  else if (i == 10)
        csr_write(PMAADDR_10, val);
  else if (i == 11)
        csr_write(PMAADDR_11, val);
  else if (i == 12)
        csr_write(PMAADDR_12, val);
  else if (i == 13)
        csr_write(PMAADDR_13, val);
  else if (i == 14)
        csr_write(PMAADDR_14, val);
  else if (i == 15)
        csr_write(PMAADDR_15, val);
}

inline unsigned long read_pmacfg(int i)
{
  unsigned long val=0;

#if __riscv_xlen == 64
  if (i == 0)
        val = csr_read(PMACFG_0);
  else if (i == 1)
        val = csr_read(PMACFG_2);
#else
  if (i == 0)
        val = csr_read(PMACFG_0);
  else if (i == 1)
        val = csr_read(PMACFG_1);
  else if (i == 2)
        val = csr_read(PMACFG_2);
  else if (i == 3)
        val = csr_read(PMACFG_3);
#endif
  return val;
}

inline void write_pmacfg(int i, unsigned long val)
{
#if __riscv_xlen == 64
  if (i == 0)
        csr_write(PMACFG_0, val);
  else if (i == 1)
        csr_write(PMACFG_2, val);
#else
  if (i == 0)
        csr_write(PMACFG_0, val);
  else if (i == 1)
        csr_write(PMACFG_1, val);
  else if (i == 2)
        csr_write(PMACFG_2, val);
  else if (i == 3)
        csr_write(PMACFG_3, val);
#endif
}
