/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 *   Nylon Chen <nylon7@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_types.h>
#include "plicsw.h"
#include "platform.h"

static u32 plicsw_ipi_hart_count;
static struct plicsw plicsw_dev[AE350_HART_COUNT_MAX];

static inline void plicsw_claim(void)
{
	u32 source_hart = current_hartid();

	plicsw_dev[source_hart].source_id =
		readl(plicsw_dev[source_hart].plicsw_claim);
}

static inline void plicsw_complete(void)
{
	u32 source_hart = current_hartid();
	u32 source = plicsw_dev[source_hart].source_id;

	writel(source, plicsw_dev[source_hart].plicsw_claim);
}

static inline void plic_sw_pending(u32 target_hart)
{
	/*
	 * The pending array registers are w1s type.
	 *
	 * We allocate a single bit for each hart.
	 * Bit 0 is hardwired to 0, thus unavailable.
	 * Bit(X+1) indicates that IPI is sent to hartX.
	 *
	 * AE350 platform guarantees only 31 interrupt sources,
	 * so target hart ID ranges from 0 to 30, else ebreak.
	 */
	if (target_hart + 1 > AE350_HART_COUNT_MAX) {
	    sbi_printf("%s: Number of harts is larger than AE350_HART_COUNT_MAX (%d)\n",
	        __func__, AE350_HART_COUNT_MAX);
	    sbi_hart_hang();
	}

	u32 pending_reg_index = (target_hart + 1) / BITS_PER_REG;
	u32 offset            = pending_reg_index * BYTES_PER_REG;
	u32 val               = 1 << ((target_hart + 1) % BITS_PER_REG);

	writel(val, (void*)plicsw_dev[target_hart].plicsw_pending + offset);

}

void plicsw_ipi_send(u32 target_hart)
{
	if (plicsw_ipi_hart_count <= target_hart)
		return;

	/* Set PLICSW IPI */
	plic_sw_pending(target_hart);
}

void plicsw_ipi_clear(u32 target_hart)
{
	if (plicsw_ipi_hart_count <= target_hart)
		return;

	/* Clear PLICSW IPI */
	plicsw_claim();
	plicsw_complete();
}

int plicsw_warm_ipi_init(void)
{
	u32 hartid = current_hartid();

	if (!plicsw_dev[hartid].plicsw_pending
	    && !plicsw_dev[hartid].plicsw_enable
	    && !plicsw_dev[hartid].plicsw_claim)
		return -1;

	/* Clear PLICSW IPI */
	plicsw_ipi_clear(hartid);

	return 0;
}

int plicsw_cold_ipi_init(unsigned long base, u32 hart_count)
{
	/* Setup source priority */
	uint32_t *priority = (void *)base + PLICSW_PRIORITY_BASE;

	for (int i = 0; i < hart_count * PLICSW_PENDING_PER_HART; i++)
		writel(1, &priority[i]);

	/* Setup target enable, bit 0 is unavailable */
	uint32_t enable_mask = 0x2;

	for (int i = 0; i < hart_count; i++) {
		uint32_t *enable = (void *)base + PLICSW_ENABLE_BASE
			+ PLICSW_ENABLE_PER_HART * i;
		writel(enable_mask, enable);
		enable_mask <<= 1;
	}

	/* Figure-out PLICSW IPI register address */
	plicsw_ipi_hart_count = hart_count;

	for (u32 hartid = 0; hartid < hart_count; hartid++) {
		plicsw_dev[hartid].source_id = 0;
		plicsw_dev[hartid].plicsw_pending =
			(void *)base
			+ PLICSW_PENDING_BASE;
		plicsw_dev[hartid].plicsw_enable  =
			(void *)base
			+ PLICSW_ENABLE_BASE
			+ PLICSW_ENABLE_PER_HART * hartid;
		plicsw_dev[hartid].plicsw_claim   =
			(void *)base
			+ PLICSW_CONTEXT_BASE
			+ PLICSW_CONTEXT_CLAIM
			+ PLICSW_CONTEXT_PER_HART * hartid;
	}

	return 0;
}
