/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *   Nylon Chen <nylon7@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_types.h>
#include <sbi/riscv_io.h>
#include "platform.h"
#include "cache.h"
#include "libfdt_env.h"

#define DCACHE_SIZE_MASK	0x7
#define DCACHE_SIZE_OFFSET	0x6
unsigned long dcache_line_size = 0;

static inline unsigned long get_cache_line_size()
{
	unsigned long mdcm_val;

	if (!dcache_line_size) {
		mdcm_val = csr_read(CSR_MDCM_CFG);
		dcache_line_size = 1UL << (((mdcm_val >> DCACHE_SIZE_OFFSET) & DCACHE_SIZE_MASK) + 2);
	}

	return dcache_line_size;
}

uintptr_t mcall_set_mcache_ctl(unsigned long input)
{
	csr_clear(CSR_MCACHE_CTL, V5_MCACHE_CTL_MASK);
	csr_write(CSR_MCACHE_CTL, input);
	return 0;
}

uintptr_t mcall_set_mmisc_ctl(unsigned long input)
{
	csr_clear(CSR_MMISC_CTL, V5_MMISC_CTL_MASK);
	csr_write(CSR_MMISC_CTL, input);
	return 0;
}

uintptr_t mcall_icache_op(unsigned int enable)
{
	if (enable)
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_IC_EN);
	else {
		csr_clear(CSR_MCACHE_CTL, V5_MCACHE_CTL_IC_EN);
		asm volatile("fence.i\n\t");
	}
	return 0;
}

uintptr_t mcall_dcache_wbinval_all(void)
{
	csr_write(CSR_MCCTLCOMMAND, V5_UCCTL_L1D_WBINVAL_ALL);

	return 0;
}

uintptr_t mcall_l1_cache_i_prefetch_op(unsigned long enable)
{
	if (enable)
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_L1I_PREFETCH_EN);
	else
		csr_clear(CSR_MCACHE_CTL, V5_MCACHE_CTL_L1I_PREFETCH_EN);

	return 0;
}

uintptr_t mcall_l1_cache_d_prefetch_op(unsigned long enable)
{
	if (enable)
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_L1D_PREFETCH_EN);
	else
		csr_clear(CSR_MCACHE_CTL, V5_MCACHE_CTL_L1D_PREFETCH_EN);

	return 0;
}

uintptr_t mcall_non_blocking_load_store(unsigned long enable)
{
	if (enable)
		csr_set(CSR_MCACHE_CTL, V5_MMISC_CTL_NON_BLOCKING_EN);
	else
		csr_clear(CSR_MCACHE_CTL, V5_MMISC_CTL_NON_BLOCKING_EN);

	return 0;
}

uintptr_t mcall_write_around(unsigned long enable)
{
	if (enable)
		csr_set(CSR_MCACHE_CTL, V5_MCACHE_CTL_DC_WAROUND_1_EN);
	else
		csr_clear(CSR_MCACHE_CTL, V5_MCACHE_CTL_DC_WAROUND_1_EN);

	return 0;
}

static uint32_t cpu_l2c_get_cctl_status(void)
{
	return readl((void*)(l2c.addr + L2C_REG_STATUS_OFFSET));
}

void cpu_dcache_inval_line(unsigned long start, unsigned long last_hartid)
{
	u32 hartid = current_hartid();

	csr_write(CSR_UCCTLBEGINADDR, start);
	csr_write(CSR_UCCTLCOMMAND, V5_UCCTL_L1D_VA_INVAL);
	if (hartid == last_hartid) {
		writel(start, (void*)((void *)l2c.addr + L2C_REG_CN_ACC_OFFSET(hartid)));
		writel(CCTL_L2_PA_INVAL, (void*)((void *)l2c.addr + L2C_REG_CN_CMD_OFFSET(hartid)));
		while ((cpu_l2c_get_cctl_status() & CCTL_L2_STATUS_CN_MASK(hartid))
			!= CCTL_L2_STATUS_IDLE);
	}
}

static void cpu_dcache_inval_range(unsigned long start, unsigned long end, unsigned long last_hartid)
{
	unsigned long line_size = get_cache_line_size();
	while (end > start) {
		cpu_dcache_inval_line(start, last_hartid);
		start += line_size;
	}
}

void cpu_dcache_wb_line(unsigned long start, unsigned long last_hartid)
{
	u32 hartid = current_hartid();

	csr_write(CSR_UCCTLBEGINADDR, start);
	csr_write(CSR_UCCTLCOMMAND, V5_UCCTL_L1D_VA_WB);
	if (hartid == last_hartid) {
		writel(start, (void*)((void *)l2c.addr + L2C_REG_CN_ACC_OFFSET(hartid)));
		writel(CCTL_L2_PA_WB, (void*)((void *)l2c.addr + L2C_REG_CN_CMD_OFFSET(hartid)));
		while ((cpu_l2c_get_cctl_status() & CCTL_L2_STATUS_CN_MASK(hartid))
			!= CCTL_L2_STATUS_IDLE);
	}
}

void cpu_dcache_wbinval_all(unsigned long start, unsigned long last_hartid)
{
	u32 hartid = current_hartid();

	csr_write(CSR_UCCTLBEGINADDR, start);
	csr_write(CSR_UCCTLCOMMAND, V5_UCCTL_L1D_WBINVAL_ALL);
	if (hartid == last_hartid) {
		writel(start, (void*)((void *)l2c.addr + L2C_REG_CN_ACC_OFFSET(hartid)));
		writel(CCTL_L2_WBINVAL_ALL, (void*)((void *)l2c.addr + L2C_REG_CN_CMD_OFFSET(hartid)));
		while ((cpu_l2c_get_cctl_status() & CCTL_L2_STATUS_CN_MASK(hartid))
			!= CCTL_L2_STATUS_IDLE);
	}
}

static void cpu_dcache_wb_range(unsigned long start, unsigned long end, unsigned long last_hartid)
{
	unsigned long line_size = get_cache_line_size();
	while (end > start) {
		cpu_dcache_wb_line(start, last_hartid);
		start += line_size;
	}
}

void cpu_dma_inval_range(unsigned long pa, unsigned long size, unsigned long last_hartid)
{
	unsigned long line_size = get_cache_line_size();
	unsigned long start = pa;
	unsigned long end = pa + size;
	unsigned long old_start = start;
	unsigned long old_end = end;
	char cache_buf[2][MAX_CACHE_LINE_SIZE];

	sbi_memset((void *)&cache_buf[0][0], 0, line_size*2);
	if (start == end)
		return;
	start = start & (~(line_size - 1));
	end = ((end + line_size - 1) & (~(line_size - 1)));
	if (start != old_start)
		sbi_memcpy((void *)&cache_buf[0][0], (void *)start, line_size);
	if (end != old_end)
		sbi_memcpy(&cache_buf[1][0], (void *)(old_end & (~(line_size - 1))), line_size);
	cpu_dcache_inval_range(start, end, last_hartid);
	if (start != old_start)
		sbi_memcpy((void *)start, &cache_buf[0][0], (old_start & (line_size - 1)));
	if (end != old_end)
		sbi_memcpy((void *)(old_end + 1), &cache_buf[1][(old_end & (line_size - 1)) + 1], end - old_end - 1);
}

void cpu_dma_wb_range(unsigned long pa, unsigned long size, unsigned long last_hartid)
{
	unsigned long start = pa;
	unsigned long end = pa + size;
	unsigned long line_size = get_cache_line_size();

	start = start & (~(line_size - 1));
	cpu_dcache_wb_range(start, end, last_hartid);
}
