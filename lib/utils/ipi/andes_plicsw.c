/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022, 2023 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 *   Nylon Chen <nylon7@andestech.com>
 *   Leo Yu-Chi Liang <ycliang@andestech.com>
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <andes/andesv5.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_ipi.h>
#include <sbi_utils/ipi/andes_plicsw.h>

struct plicsw_data plicsw;

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

	writel(val, (void *)plicsw.addr + PLICSW_PENDING_BASE + offset);
}

static void plicsw_ipi_send(u32 target_hart)
{
	if (plicsw.hart_count <= target_hart)
		ebreak();

	/* Set PLICSW IPI */
	plic_sw_pending(target_hart);
}

static void plicsw_ipi_clear(u32 target_hart)
{
	u32 hartid = current_hartid();

	if (plicsw.hart_count <= target_hart)
		ebreak();

	/* Claim */
	u32 source = readl((void *)plicsw.addr + PLICSW_CONTEXT_BASE +
				PLICSW_CONTEXT_CLAIM + PLICSW_CONTEXT_STRIDE * hartid);
	/* Complete */
	writel(source, (void *)plicsw.addr + PLICSW_CONTEXT_BASE +
		PLICSW_CONTEXT_CLAIM + PLICSW_CONTEXT_STRIDE * hartid);
}

static struct sbi_ipi_device plicsw_ipi = {
	.name      = "andes_plicsw",
	.ipi_send  = plicsw_ipi_send,
	.ipi_clear = plicsw_ipi_clear
};

int plicsw_warm_ipi_init(void)
{
	u32 hartid = current_hartid();

	/* Clear PLICSW IPI */
	plicsw_ipi_clear(hartid);

	return 0;
}

int plicsw_cold_ipi_init(struct plicsw_data *plicsw)
{
	int rc;

	/* Setup source priority */
	uint32_t *priority = (void *)plicsw->addr + PLICSW_PRIORITY_BASE;

	for (int i = 0; i < plicsw->hart_count; i++)
		writel(1, &priority[i]);

	/* Setup target enable, bit 0 is unavailable */
	uint32_t enable_mask = 0x2;

	for (int i = 0; i < plicsw->hart_count; i++) {
		uint32_t *enable = (void *)plicsw->addr + PLICSW_ENABLE_BASE +
				   PLICSW_ENABLE_STRIDE * i;
		writel(enable_mask, enable);
		enable_mask <<= 1;
	}

	/* Add PLICSW region to the root domain */
	rc = sbi_domain_root_add_memrange(plicsw->addr, plicsw->size,
					  PLICSW_REGION_ALIGN,
					  SBI_DOMAIN_MEMREGION_MMIO);
	if (rc)
		return rc;

	sbi_ipi_set_device(&plicsw_ipi);

	return 0;
}
