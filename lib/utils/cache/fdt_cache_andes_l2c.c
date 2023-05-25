/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_types.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/cache/fdt_cache.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <andes/andesv5.h>
#include <andes/l2c_hpm.h>

/* clang-format off */

/* L2 cache registers */
#define L2C_REG_CFG_OFFSET		0
#define L2C_REG_CTL_OFFSET		0x8
#define L2C_HPM_CTL_OFFSET		0x10
#define L2C_REG_C0_CMD_OFFSET		0x40
#define L2C_REG_C0_ACC_OFFSET		0x48
#define L2C_REG_C1_CMD_OFFSET		0x50
#define L2C_REG_C1_ACC_OFFSET		0x58
#define L2C_REG_C2_CMD_OFFSET		0x60
#define L2C_REG_C2_ACC_OFFSET		0x68
#define L2C_REG_C3_CMD_OFFSET		0x70
#define L2C_REG_C3_ACC_OFFSET		0x78
#define L2C_REG_C0_STATUS_OFFSET	0x80
#define L2C_REG_C0_HPM_OFFSET		0x200

/* Per-hart offsets */
#define CCTL_CMD_REG(hart)	(L2C_REG_C0_CMD_OFFSET + (hart) * (l2c.cmd_stride))
#define CCTL_STATUS_REG(hart)	(L2C_REG_C0_STATUS_OFFSET + (hart) * (l2c.status_stride))
#define CCTL_L2_STATUS_C0_MASK	0xF
#define CCTL_L2_STATUS_CN_MASK(hart)	\
	(CCTL_L2_STATUS_C0_MASK << ((hart) * l2c.status_bit_offset))

/* Configuration register encoding */
#define MEM_MAP_OFF	20
#define MEM_MAP_MSK	BIT(MEM_MAP_OFF)

/* Control register encoding */
#define L2_ENABLE	0x1

/* L2 CCTL status */
#define CCTL_L2_STATUS_IDLE		0
#define CCTL_L2_STATUS_PROCESS		1
#define CCTL_L2_STATUS_ILLEGAL		2

/* L2 cache operation */
#define CCTL_L2_WBINVAL_ALL		0x12

/* L2 HPM Control Selector */
#define L2C_HPM_CTL_SEL0_MSK	0xff
#define L2C_EVT_ACCESSES	0x01
#define L2C_EVT_ACCESS_MISSES	0x02
#define L2C_EVT_NONE		0xff

/* clang-format on */

struct l2c_data {
	unsigned long addr;
	/* L2C Gen1/Gen2 quirks */
	unsigned int cmd_stride;
	unsigned int status_stride;
	unsigned int status_bit_offset;
};

static struct l2c_data l2c;

static int andes_l2c_enable(void)
{
	u32 ctr;

	ctr = readl((void *)(l2c.addr + L2C_REG_CTL_OFFSET));
	if (!(ctr & L2_ENABLE))
		writel(ctr | L2_ENABLE,
		       (void *)(l2c.addr + L2C_REG_CTL_OFFSET));

	/* Check if l2c is enabled */
	return (readl((void *)(l2c.addr + L2C_REG_CTL_OFFSET)) & L2_ENABLE)
		? 0
		: SBI_EINVAL;
}

static inline uint32_t l2c_get_cctl_status(u32 hartid)
{
	return readl((void *)(l2c.addr + CCTL_STATUS_REG(hartid)));
}

static int andes_l2c_wbinval_all(void)
{
	u32 hartid;

	if (!(readl((void *)(l2c.addr + L2C_REG_CTL_OFFSET)) & L2_ENABLE))
		return SBI_EINVAL; /* l2c is not enabled */

	/*
	 * All harts share the same L2 cache. Selecting the current hart
	 * to do the WBINVAL.
	 */
	hartid = current_hartid();
	writel(CCTL_L2_WBINVAL_ALL, (void *)(l2c.addr + CCTL_CMD_REG(hartid)));
	/* Wait for L2C to complete the command */
	while ((l2c_get_cctl_status(hartid) & CCTL_L2_STATUS_CN_MASK(hartid)) !=
	       CCTL_L2_STATUS_IDLE)
		;

	return 0;
}

static int andes_l2c_get_addr(unsigned long *addr)
{
	*addr = l2c.addr;
	return 0;
}

static int andes_l2c_read_hpm_ctr(u64 *out_val)
{
	u32 vall = readl((void *)(l2c.addr + L2C_REG_C0_HPM_OFFSET));
	u32 valh = readl((void *)(l2c.addr + L2C_REG_C0_HPM_OFFSET + 4));
	*out_val = ((u64)valh << 32) | vall;
	return 0;
}

static int andes_l2c_write_hpm_ctr(u64 val)
{
	writel((u32)val, (void *)(l2c.addr + L2C_REG_C0_HPM_OFFSET));
	writel((u32)(val >> 32),
	       (void *)(l2c.addr + L2C_REG_C0_HPM_OFFSET + 4));
	return 0;
}

static int andes_l2c_start_hpm(uint32_t event_idx_code)
{
	u32 hpm_ctl;

	hpm_ctl = readl((void *)(l2c.addr + L2C_HPM_CTL_OFFSET));

	switch (event_idx_code) {
	case SBI_PMU_FW_L2C_ACCESSES: /* event_idx: 0xf0016 */
		hpm_ctl = INSERT_FIELD(hpm_ctl, L2C_HPM_CTL_SEL0_MSK,
				       L2C_EVT_ACCESSES);
		break;
	case SBI_PMU_FW_L2C_ACCESS_MISSES: /* event_idx: 0xf0017 */
		hpm_ctl = INSERT_FIELD(hpm_ctl, L2C_HPM_CTL_SEL0_MSK,
				       L2C_EVT_ACCESS_MISSES);
		break;
	default:
		return SBI_EINVAL;
	}

	writel(hpm_ctl, (void *)(l2c.addr + L2C_HPM_CTL_OFFSET));

	return 0;
}

static int andes_l2c_stop_hpm(void)
{
	u32 hpm_ctl;

	hpm_ctl = readl((void *)(l2c.addr + L2C_HPM_CTL_OFFSET));
	hpm_ctl = INSERT_FIELD(hpm_ctl, L2C_HPM_CTL_SEL0_MSK, L2C_EVT_NONE);
	writel(hpm_ctl, (void *)(l2c.addr + L2C_HPM_CTL_OFFSET));

	return 0;
}

static bool andes_l2c_hpm_idle(void)
{
	u32 hpm_ctl;

	hpm_ctl = readl((void *)(l2c.addr + L2C_HPM_CTL_OFFSET));

	return EXTRACT_FIELD(hpm_ctl, L2C_HPM_CTL_SEL0_MSK) == L2C_EVT_NONE;
}

static struct cache andes_l2c = {
	.enable	     = andes_l2c_enable,
	.wbinval_all = andes_l2c_wbinval_all,
	.get_addr    = andes_l2c_get_addr,
	/*
	 * L2C HPM helpers (Andes-specific)
	 */
	.read_hpm_ctr = andes_l2c_read_hpm_ctr,
	.write_hpm_ctr = andes_l2c_write_hpm_ctr,
	.start_hpm    = andes_l2c_start_hpm,
	.stop_hpm     = andes_l2c_stop_hpm,
	.hpm_idle     = andes_l2c_hpm_idle
};

static int andes_l2c_init(void *fdt, int nodeoff, const struct fdt_match *match)
{
	int rc;
	uint64_t addr;
	ulong mmsc_cfg;

	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, NULL);
	if (rc)
		return rc;

	/* On 45 and newer CPU series, we can also check with mmsc_cfg.L2C bit */
	if (!(is_andes(25) || is_andes(27))) {
#if __riscv_xlen == 64
		mmsc_cfg = csr_read(CSR_MMSC_CFG);
		if (!EXTRACT_FIELD(mmsc_cfg, CSR_MMSC_CFG_L2C_MASK)) {
#else
		mmsc_cfg = csr_read(CSR_MMSC_CFG2);
		if (!EXTRACT_FIELD(mmsc_cfg, CSR_MMSC_CFG2_L2C_MASK)) {
#endif
			sbi_printf(
				"%s: WARNING: The Andes platform doesn't support L2C, the l2-cache@%lx node should not be valid.\n",
				__func__, (unsigned long)addr);
			return SBI_ENODEV;
		}
	}

	l2c.addr = (unsigned long)addr;
	if (readl((void *)(l2c.addr + L2C_REG_CFG_OFFSET)) & MEM_MAP_MSK) {
		/* v1 memory map (Gen2) */
		l2c.cmd_stride	      = 0x1000;
		l2c.status_stride     = 0x1000;
		l2c.status_bit_offset = 0x0;
	} else {
		/* v0 memory map (Gen1) */
		l2c.cmd_stride	      = 0x10;
		l2c.status_stride     = 0x0;
		l2c.status_bit_offset = 0x4;
	}

	rc = cache_add(&andes_l2c);
	if (rc)
		return rc;

	return 0;
}

static const struct fdt_match andes_l2c_match[] = {
	{ .compatible = "cache" },
	{},
};

struct fdt_cache fdt_cache_andes_l2c = {
	.match_table = andes_l2c_match,
	.init	     = andes_l2c_init,
};
