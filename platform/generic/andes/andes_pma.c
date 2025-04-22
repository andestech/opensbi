// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 * Copyright (c) 2024 Andes Technology Corporation
 *
 * Authors:
 *      Ben Zong-You Xie <ben717@andestech.com>
 *      Lad Prabhakar <prabhakar.mahadev-lad.rj@bp.renesas.com>
 */

#include <andes/andes.h>
#include <andes/andes_pma.h>
#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/fdt/fdt_helper.h>

static unsigned long andes_pma_read_num(unsigned int csr_num)
{
#define switchcase_csr_read(__csr_num, __val)		\
	case __csr_num:					\
		__val = csr_read(__csr_num);		\
		break;
#define switchcase_csr_read_2(__csr_num, __val)		\
	switchcase_csr_read(__csr_num + 0, __val)	\
	switchcase_csr_read(__csr_num + 1, __val)
#define switchcase_csr_read_4(__csr_num, __val)		\
	switchcase_csr_read_2(__csr_num + 0, __val)	\
	switchcase_csr_read_2(__csr_num + 2, __val)
#define switchcase_csr_read_8(__csr_num, __val)		\
	switchcase_csr_read_4(__csr_num + 0, __val)	\
	switchcase_csr_read_4(__csr_num + 4, __val)
#define switchcase_csr_read_16(__csr_num, __val)	\
	switchcase_csr_read_8(__csr_num + 0, __val)	\
	switchcase_csr_read_8(__csr_num + 8, __val)

	unsigned long ret = 0;

	switch (csr_num) {
	switchcase_csr_read_4(CSR_PMACFG0, ret)
	switchcase_csr_read_4(CSR_PMACFG0 + 4, ret)
	switchcase_csr_read_4(CSR_PMACFG0 + 8, ret)
	switchcase_csr_read_16(CSR_PMAADDR0, ret)
	switchcase_csr_read_16(CSR_PMAADDR0 + 16, ret)
	switchcase_csr_read_16(CSR_PMAADDR0 + 32, ret)
	default:
		sbi_panic("%s: Unknown Andes PMA CSR %#x", __func__, csr_num);
		break;
	}

	return ret;

#undef switchcase_csr_read_16
#undef switchcase_csr_read_8
#undef switchcase_csr_read_4
#undef switchcase_csr_read_2
#undef switchcase_csr_read
}

static void andes_pma_write_num(unsigned int csr_num, unsigned long val)
{
#define switchcase_csr_write(__csr_num, __val)		\
	case __csr_num:					\
		csr_write(__csr_num, __val);		\
		break;
#define switchcase_csr_write_2(__csr_num, __val)	\
	switchcase_csr_write(__csr_num + 0, __val)	\
	switchcase_csr_write(__csr_num + 1, __val)
#define switchcase_csr_write_4(__csr_num, __val)	\
	switchcase_csr_write_2(__csr_num + 0, __val)	\
	switchcase_csr_write_2(__csr_num + 2, __val)
#define switchcase_csr_write_8(__csr_num, __val)	\
	switchcase_csr_write_4(__csr_num + 0, __val)	\
	switchcase_csr_write_4(__csr_num + 4, __val)
#define switchcase_csr_write_16(__csr_num, __val)	\
	switchcase_csr_write_8(__csr_num + 0, __val)	\
	switchcase_csr_write_8(__csr_num + 8, __val)

	switch (csr_num) {
	switchcase_csr_write_4(CSR_PMACFG0, val)
	switchcase_csr_write_4(CSR_PMACFG0 + 4, val)
	switchcase_csr_write_4(CSR_PMACFG0 + 8, val)
	switchcase_csr_write_16(CSR_PMAADDR0, val)
	switchcase_csr_write_16(CSR_PMAADDR0 + 16, val)
	switchcase_csr_write_16(CSR_PMAADDR0 + 32, val)
	default:
		sbi_panic("%s: Unknown Andes PMA CSR %#x", __func__, csr_num);
		break;
	}

#undef switchcase_csr_write_16
#undef switchcase_csr_write_8
#undef switchcase_csr_write_4
#undef switchcase_csr_write_2
#undef switchcase_csr_write
}

static bool not_napot(unsigned long addr, unsigned long size)
{
	return ((size & (size - 1)) || (addr & (size - 1)));
}

static bool is_pma_entry_disable(char pmaxcfg)
{
	return (pmaxcfg & ANDES_PMACFG_ETYP_MASK) == ANDES_PMACFG_ETYP_OFF ?
	       true : false;
}

union pmacfgx{
        unsigned long val;
#if __riscv_xlen == 64
        char pmaxcfg[8];
#elif __riscv_xlen == 32
        char pmaxcfg[4];
#else
#error "Unexpected __riscv_xlen"
#endif
};

static char get_pmaxcfg(int entry_id)
{
	unsigned int pmacfgx_addr;
	unsigned int idx;
	union pmacfgx pmacfgx;

#if __riscv_xlen == 64
	pmacfgx_addr = CSR_PMACFG0 + ((entry_id / 8) << 1);
	idx = entry_id % 8;
#elif __riscv_xlen == 32
	pmacfgx_addr = CSR_PMACFG0 + (entry_id / 4);
	idx = entry_id % 4;
#else
#error "Unexpected __riscv_xlen"
#endif
	pmacfgx.val = andes_pma_read_num(pmacfgx_addr);
	return pmacfgx.pmaxcfg[idx];
}

static void set_pmaxcfg(int entry_id, char flags)
{
	unsigned int pmacfgx_addr;
	unsigned int idx;
	union pmacfgx pmacfgx;

#if __riscv_xlen == 64
	pmacfgx_addr = CSR_PMACFG0 + ((entry_id / 8) << 1);
	idx = entry_id % 8;
#elif __riscv_xlen == 32
	pmacfgx_addr = CSR_PMACFG0 + (entry_id / 4);
	idx = entry_id % 4;
#else
#error "Unexpected __riscv_xlen"
#endif
	pmacfgx.val = andes_pma_read_num(pmacfgx_addr);
	pmacfgx.pmaxcfg[idx] = flags;
	andes_pma_write_num(pmacfgx_addr, pmacfgx.val);
}

static void decode_pmaaddrx(int entry_id, unsigned long *out_start,
			    unsigned long *out_size)
{
	unsigned long pmaaddr;
	int k;

	/*
	 * (Assume $pmaaddr would never has value of all bits set)
	 * Given $pmaaddr, let k = # of trailing 1s of $pmaaddr
	 * out_size  = 2 ^ (k + 3)
	 * out_start = 4 * ($pmaaddr + 1 - (out_size / 8))
	 */

	pmaaddr = andes_pma_read_num(CSR_PMAADDR0 + entry_id);
	k = sbi_ffz(pmaaddr);
	*out_size = 1 << (k + 3);
	*out_start = (pmaaddr + 1 - (*out_size >> 3)) << 2;
}

static unsigned long encode_pmaaddrx(unsigned long addr, unsigned long size)
{
	return (addr >> 2) + (size >> 3) - 1;
}

static bool has_pma_region_overlap(unsigned long start, unsigned long size,
				   int *conflict_id)
{
	unsigned long end = start + size - 1;
	unsigned long _start, _size, _end;
	char pmaxcfg;
	int ppma_amount = pma_probe_ver() == PPMA_VERSION_48_ENRTY ? 48 : 16;

	for (int i = 0; i < ppma_amount; i++) {
		pmaxcfg = get_pmaxcfg(i);
		if (is_pma_entry_disable(pmaxcfg))
			continue;

		decode_pmaaddrx(i, &_start, &_size);
		_end = _start + _size - 1;

		if (MAX(start, _start) <= MIN(end, _end)) {
			sbi_printf(
				"ERROR %s(): %#lx ~ %#lx overlaps with PMA%d: %#lx ~ %#lx\n",
				__func__, start, end, i, _start, _end);
			*conflict_id = i;
			return true;
		}
	}

	return false;
}

static unsigned long andes_pma_setup(const struct andes_pma_region *pma_region,
				     unsigned int entry_id)
{
	unsigned long size = pma_region->size;
	unsigned long addr = pma_region->pa;
	unsigned char flags = pma_region->flags;
	unsigned long pmaaddr;

	/* Check for a 4KiB granularity NAPOT region */
	if (size < ANDES_PMA_GRANULARITY || not_napot(addr, size) ||
	    !(flags & ANDES_PMACFG_ETYP_NAPOT))
		return SBI_EINVAL;

	set_pmaxcfg(entry_id, flags);

	pmaaddr = encode_pmaaddrx(addr, size);

	andes_pma_write_num(CSR_PMAADDR0 + entry_id, pmaaddr);

	return andes_pma_read_num(CSR_PMAADDR0 + entry_id) == pmaaddr ?
	       pmaaddr : SBI_EINVAL;
}

static int andes_fdt_pma_resv(void *fdt, const struct andes_pma_region *pma,
			      unsigned int index, int parent)
{
	int na = fdt_address_cells(fdt, 0);
	int ns = fdt_size_cells(fdt, 0);
	static bool dma_default = false;
	fdt32_t addr_high, addr_low;
	fdt32_t size_high, size_low;
	int subnode, err;
	fdt32_t reg[4];
	fdt32_t *val;
	char name[32];

	addr_high = (u64)pma->pa >> 32;
	addr_low = pma->pa;
	size_high = (u64)pma->size >> 32;
	size_low = pma->size;

	if (na > 1 && addr_high) {
		sbi_snprintf(name, sizeof(name),
			     "pma_resv%d@%x,%x",
			     index, addr_high, addr_low);
	} else {
		sbi_snprintf(name, sizeof(name),
			     "pma_resv%d@%x",
			     index, addr_low);
	}
	subnode = fdt_add_subnode(fdt, parent, name);
	if (subnode < 0)
		return subnode;

	if (pma->shared_dma) {
		err = fdt_setprop_string(fdt, subnode, "compatible",
					 "shared-dma-pool");
		if (err < 0)
			return err;
	}

	if (pma->no_map) {
		err = fdt_setprop_empty(fdt, subnode, "no-map");
		if (err < 0)
			return err;
	}

	/* Linux allows single linux,dma-default region. */
	if (pma->dma_default) {
		if (dma_default)
			return SBI_EINVAL;

		err = fdt_setprop_empty(fdt, subnode, "linux,dma-default");
		if (err < 0)
			return err;
		dma_default = true;
	}

	/* Encode the <reg> property value */
	val = reg;
	if (na > 1)
		*val++ = cpu_to_fdt32(addr_high);
	*val++ = cpu_to_fdt32(addr_low);
	if (ns > 1)
		*val++ = cpu_to_fdt32(size_high);
	*val++ = cpu_to_fdt32(size_low);

	err = fdt_setprop(fdt, subnode, "reg", reg,
			  (na + ns) * sizeof(fdt32_t));
	if (err < 0)
		return err;

	return 0;
}

static int andes_fdt_reserved_memory_fixup(void *fdt,
					   const struct andes_pma_region *pma,
					   unsigned int entry)
{
	int parent;

	/* Try to locate the reserved memory node */
	parent = fdt_path_offset(fdt, "/reserved-memory");
	if (parent < 0) {
		int na = fdt_address_cells(fdt, 0);
		int ns = fdt_size_cells(fdt, 0);
		int err;

		/* If such node does not exist, create one */
		parent = fdt_add_subnode(fdt, 0, "reserved-memory");
		if (parent < 0)
			return parent;

		err = fdt_setprop_empty(fdt, parent, "ranges");
		if (err < 0)
			return err;

		err = fdt_setprop_u32(fdt, parent, "#size-cells", ns);
		if (err < 0)
			return err;

		err = fdt_setprop_u32(fdt, parent, "#address-cells", na);
		if (err < 0)
			return err;
	}

	return andes_fdt_pma_resv(fdt, pma, entry, parent);
}

int andes_pma_setup_regions(const struct andes_pma_region *pma_regions,
			    unsigned int pma_regions_count)
{
	unsigned long mmsc = csr_read(CSR_MMSC_CFG);
	unsigned int dt_populate_cnt;
	unsigned int i, j;
	unsigned long pa;
	void *fdt;
	int ret;
	unsigned int ppma_amount =
		pma_probe_ver() == PPMA_VERSION_48_ENRTY ? 48 : 16;

	if (!pma_regions || !pma_regions_count)
		return 0;

	if (pma_regions_count > ppma_amount)
		return SBI_EINVAL;

	if ((mmsc & MMSC_CFG_PPMA_MASK) == 0)
		return SBI_ENOTSUPP;

	/* Configure the PMA regions */
	dt_populate_cnt = 0;
	for (i = 0; i < pma_regions_count; i++) {
		pa = andes_pma_setup(&pma_regions[i], i);
		if (pa == SBI_EINVAL)
			return SBI_EINVAL;
		else if (pma_regions[i].dt_populate)
			dt_populate_cnt++;
	}

	if (!dt_populate_cnt)
		return 0;

	fdt = fdt_get_address();

	ret = fdt_open_into(fdt, fdt,
			    fdt_totalsize(fdt) + (64 * dt_populate_cnt));
	if (ret < 0)
		return ret;

	for (i = 0, j = 0; i < pma_regions_count; i++) {
		if (!pma_regions[i].dt_populate)
			continue;

		ret = andes_fdt_reserved_memory_fixup(fdt,
						      &pma_regions[i],
						      j++);
		if (ret)
			return ret;
	}

	return 0;
}

static unsigned long pma_features_offset;

static virtual_addr_t get_pma_table(unsigned int pma_idx)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct andes_pma_data *pma_data =
		sbi_scratch_offset_ptr(scratch, pma_features_offset);

	if (!pma_data)
		return 0;

	return pma_data->pma_user_table[pma_idx];
}

static int set_pma_table(int pma_idx, virtual_addr_t va)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct andes_pma_data *pma_data =
		sbi_scratch_offset_ptr(scratch, pma_features_offset);

	if (!pma_data)
		return SBI_EINVAL;

	pma_data->pma_user_table[pma_idx] = va;

	return SBI_OK;
}

static bool is_va_alias(unsigned long va)
{
	/**
	 * Check if the va conflicts with the existing one in
	 * pma_user_table[]
	 */
	int ppma_amount = pma_probe_ver() == PPMA_VERSION_48_ENRTY ? 48 : 16;

	for (int i = 0; i < ppma_amount; i++) {
		if (get_pma_table(i) != va)
			continue;
		sbi_printf_highlight("ERROR %s(): va %#lx conflicts\n",
				     __func__, va);
		return true;
	}

	return false;
}

static int allocate_pma_entry(unsigned long va, int *entry_id)
{
	char pmaxcfg;
	int ppma_amount = pma_probe_ver() == PPMA_VERSION_48_ENRTY ? 48 : 16;

	/* Not allow va alias */
	if(is_va_alias(va)) {
		sbi_printf_highlight(
			"ERROR %s(): Cannot allocate PMA entry for %#lx\n",
			__func__, va);

		*entry_id = -1;
		return SBI_ENOENT;
	}

	for (int i = 0; i < ppma_amount; i++) {
		pmaxcfg = get_pmaxcfg(i);
		if (is_pma_entry_disable(pmaxcfg)) {
			set_pma_table(i, va);
			*entry_id = i;
			return SBI_OK;
		}
	}
	return SBI_ENOENT;
}

bool mcall_probe_pma(void)
{
	return !!EXTRACT_FIELD(csr_read(CSR_MMSC_CFG), MMSC_CFG_PPMA_MASK);
}

int mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size)
{
	int conflict_id = -1;
	int entry_id;
	int rc;
	unsigned long pmaaddr;
	char pmaxcfg;

	/* Sanity check */
	if (!mcall_probe_pma()) {
		sbi_printf_highlight(
			"ERROR %s(): platform does not support PPMA.\n",
			__func__);
		return SBI_ERR_NOT_SUPPORTED;
	}

	if (!va) {
		sbi_printf_highlight(
			"ERROR %s(): expecting non-zero va\n", __func__);
		return SBI_EFAIL;
	}

	/* Check size is 4KiB granularity and is power of 2 */
	if (size < ANDES_PMA_GRANULARITY || not_napot(pa, size)) {
		sbi_printf_highlight(
			"ERROR %s(): %#lx - %#lx (size: %#lx) is not a 4KiB granularity NAPOT region\n",
			__func__, pa, pa + size - 1, size);
		return SBI_ERR_INVALID_PARAM;
	}

	if (has_pma_region_overlap(pa, size, &conflict_id)) {
		sbi_printf_highlight(
			"ERROR %s(): PMA region overlaps with PMA%d used by va=%#lx\n",
			__func__, conflict_id,
			get_pma_table(conflict_id));
		return SBI_ERR_INVALID_PARAM;
	}

	rc = allocate_pma_entry(va, &entry_id);
	if (rc)
		return SBI_ENOENT;

	pmaxcfg = ANDES_PMACFG_ETYP_NAPOT | ANDES_PMACFG_MTYP_MEM_NON_CACHE_BUF;
	set_pmaxcfg(entry_id, pmaxcfg);

	pmaaddr = encode_pmaaddrx(pa, size);
	andes_pma_write_num(CSR_PMAADDR0 + entry_id, pmaaddr);

	if (andes_pma_read_num(CSR_PMAADDR0 + entry_id) != pmaaddr) {
		sbi_printf_highlight(
			"ERROR %s(): Failed to set the pmaaddr%d to desired value\n",
			__func__, entry_id);
		return SBI_EFAIL;
	}

	return SBI_OK;
}

int mcall_free_pma(unsigned long va)
{
	char pmaxcfg;
	int ppma_amount = pma_probe_ver() == PPMA_VERSION_48_ENRTY ? 48 : 16;
	int entry_id;

	/* Sanity check */
	if (!mcall_probe_pma()) {
		sbi_printf_highlight(
			"ERROR %s(): platform does not support PPMA.\n",
			__func__);
		return SBI_ERR_NOT_SUPPORTED;
	}

	if (!va) {
		sbi_printf_highlight(
			"ERROR %s(): expecting non-zero va\n", __func__);
		return SBI_EFAIL;
	}

	for (entry_id = 0; entry_id < ppma_amount; entry_id++) {
		if (get_pma_table(entry_id) == va)
			break;
	}

	if (entry_id == ppma_amount) {
		sbi_dprintf(
			"ERROR %s(): va %#lx has never registered with PPMA\n",
			__func__, va);
		return SBI_EFAIL;
	}

	pmaxcfg = get_pmaxcfg(entry_id);
	if (is_pma_entry_disable(pmaxcfg)) {
		sbi_printf_highlight(
			"ERROR %s(): expecting PMA%d is enabled for %#lx\n",
			__func__, entry_id, va);

		return SBI_EFAIL;
	}

	set_pmaxcfg(entry_id, ANDES_PMACFG_ETYP_OFF);
	andes_pma_write_num(CSR_PMAADDR0 + entry_id, 0x0);

	/* Free the entry */
	set_pma_table(entry_id, 0x0);

	return SBI_OK;
}

/*
 * For rv64:
 * 	mmsc_cfg.MSC_EXT3 (bit-63) determine whether mmsc_cfg3 exist
 *	mmsc_cfg3.PPMA_VER (bit-32_34) determine the PPMA version
 * For rv32:
 * 	mmsc_cfg.MSC_EXT   (bit-31) determine whether mmsc_cfg2 exist
 * 	mmsc_cfg2.MSC_EXT3 (bit-31) determine whether mmsc_cfg3 exist
 * 	mmsc_cfg3.MSC_EXT4 (bit-31) determine whether mmsc_cfg4 exist
 * 	mmsc_cfg4.PPMA_VER (bit-0_2) determine the PPMA version
 * For the value of PPMA_VER:
 * 	0: 16 entry version -> PPMA_VERSION_16_ENRTY
 * 	2: 48 entry version -> PPMA_VERSION_48_ENRTY
 * 	others: reserved
 *
 * Since currrently only 2 legal versions exist, a single bit is used to
 * determine the version (i.e., the middle bit of the field PPMA_VER for
 * the value 0 (b000) or value 2 (b010)).
 */
int pma_probe_ver()
{
#if __riscv_xlen == 64
	if (csr_read(CSR_MMSC_CFG) & BIT(63) &&
	    csr_read(CSR_MMSC_CFG3) & BIT(33))
	    	return PPMA_VERSION_48_ENRTY;
#else
	if (csr_read(CSR_MMSC_CFG) & BIT(31) &&
	    csr_read(CSR_MMSC_CFG2) & BIT(31) &&
	    csr_read(CSR_MMSC_CFG3) & BIT(31) &&
	    csr_read(CSR_MMSC_CFG4) & BIT(1))
		return PPMA_VERSION_48_ENRTY;
#endif
	else
		return PPMA_VERSION_16_ENRTY;
}

int pma_init(void)
{
	/* Allocate PMA mapping table in every hart's scratch region */
	pma_features_offset = sbi_scratch_alloc_offset(
					sizeof(struct andes_pma_data));
	if (!pma_features_offset)
		return SBI_ENOMEM;

	return SBI_OK;
}
