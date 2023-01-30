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
	switchcase_csr_read_16(CSR_PMAADDR0, ret)
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
	switchcase_csr_write_16(CSR_PMAADDR0, val)
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

static inline bool not_napot(unsigned long addr, unsigned long size)
{
	return ((size & (size - 1)) || (addr & (size - 1)));
}

static unsigned long andes_pma_setup(const struct andes_pma_region *pma_region,
				     unsigned int entry_id)
{
	unsigned long size = pma_region->size;
	unsigned long addr = pma_region->pa;
	unsigned int pma_cfg_addr;
	unsigned long pmacfg_val;
	unsigned long pmaaddr;
	char *pmaxcfg;

	/* Check for a 4KiB granularity NAPOT region*/
	if (size < ANDES_PMA_GRANULARITY || not_napot(addr, size) ||
	    !(pma_region->flags & ANDES_PMACFG_ETYP_NAPOT))
		return SBI_EINVAL;

#if __riscv_xlen == 64
	pma_cfg_addr = CSR_PMACFG0 + ((entry_id / 8) ? 2 : 0);
	pmacfg_val = andes_pma_read_num(pma_cfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 8);
#elif __riscv_xlen == 32
	pma_cfg_addr = CSR_PMACFG0 + (entry_id / 4);
	pmacfg_val = andes_pma_read_num(pma_cfg_addr);
	pmaxcfg = (char *)&pmacfg_val + (entry_id % 4);
#else
#error "Unexpected __riscv_xlen"
#endif
	*pmaxcfg = pma_region->flags;

	andes_pma_write_num(pma_cfg_addr, pmacfg_val);

	pmaaddr = (addr >> 2) + (size >> 3) - 1;

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

	if (!pma_regions || !pma_regions_count)
		return 0;

	if (pma_regions_count > ANDES_MAX_PMA_REGIONS)
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

static inline virtual_addr_t get_pma_table(int pma_idx)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct andes_pma_data *pma_data =
		sbi_scratch_offset_ptr(scratch, pma_features_offset);

	if (!pma_data)
		return 0;

	return pma_data->pma_user_table[pma_idx];
}

static inline int set_pma_table(int pma_idx, virtual_addr_t va)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	struct andes_pma_data *pma_data =
		sbi_scratch_offset_ptr(scratch, pma_features_offset);

	if (!pma_data)
		return SBI_EINVAL;

	pma_data->pma_user_table[pma_idx] = va;
	return SBI_OK;
}

static unsigned long read_pmaaddrx(int entry_id)
{
	return andes_pma_read_num(CSR_PMAADDR0 + entry_id);
}

static char read_pmaxcfg(int entry_id)
{
	unsigned long pmacfg_val;
	unsigned long pmaxcfg_mask;

	if (entry_id < 0 || entry_id >= ANDES_MAX_PMA_REGIONS) {
		sbi_printf_highlight("ERROR %s(): non-existing PMA entry_id: %d\n",
			  __func__, entry_id);
		return 0;
	}


#if __riscv_xlen == 64
	pmacfg_val = andes_pma_read_num(CSR_PMACFG0 + ((entry_id / 8) ? 2 : 0));
	pmaxcfg_mask = 0xfful << (8 * (entry_id % 8));
#else
	pmacfg_val   = andes_pma_read_num(CSR_PMACFG0 + (entry_id / 4));
	pmaxcfg_mask = 0xfful << (8 * (entry_id % 4));
#endif

	return EXTRACT_FIELD(pmacfg_val, pmaxcfg_mask);
}

static bool is_va_alias(unsigned long va)
{
	/**
	 * Check if the va conflicts with the existing one in
	 * pma_user_table[]
	 */
	for (int i = 0; i < ANDES_MAX_PMA_REGIONS; i++) {
		if (get_pma_table(i) != va)
			continue;
		sbi_printf_highlight("ERROR %s(): va %#lx conflicts\n", __func__,
			  va);
		return true;
	}
	return false;
}

static u8 pmaxcfg_etyp(int entry_id)
{
	/* pmaxcfg.ETYP will be 0 when it is set to 1 or 2 (reserved) */
	u8 pmaxcfg_val = read_pmaxcfg(entry_id);
	return EXTRACT_FIELD(pmaxcfg_val, PMACFG_ETYP_MASK);
}

static int allocate_pma_entry(unsigned long va, int *entry_id)
{
	/* Not allow va alias */
	if(is_va_alias(va))
		goto fail;

	for (int i = 0; i < ANDES_MAX_PMA_REGIONS; i++) {
		if (pmaxcfg_etyp(i) != PMACFG_ETYP_OFF)
			continue;
		set_pma_table(i, va);
		*entry_id	  = i;
		return SBI_SUCCESS;
	}

fail:
	sbi_printf_highlight("ERROR %s(): Cannot allocate PMA entry for %#lx\n", __func__,
		   va);
	*entry_id = -1;

	return SBI_ENOENT;
}

bool mcall_probe_pma(void)
{
	return !!EXTRACT_FIELD(csr_read(CSR_MMSC_CFG), MMSC_CFG_PPMA_MASK);
}

static int pmaaddrx_to_region(int entry_id, unsigned long *out_start,
			      unsigned long *out_size)
{
	/*
	 * Given entry id, calculate the region start and size
	 * @entry_id the PMA entry id (only 0 ~ 15)
	 *
	 * @out_start output value for start address
	 * @out_size output value for region size
	 *
	 * Return 0 on success
	 * Return SBI_EINVAL if PMA is not in NAPOT mode
	 */
	unsigned long pmaaddrx_val;
	int k;

	if (pmaxcfg_etyp(entry_id) == PMACFG_ETYP_NAPOT) {
		pmaaddrx_val = read_pmaaddrx(entry_id);
		/**
		* Given $pmaaddr, let k = # of trailing 1s
		* size = 2^(k + 3)
		* base = 4 * ($pmaaddr - (size / 8) + 1)
		*/
		k     = sbi_ffz(pmaaddrx_val);
		*out_size  = 1 << (k + 3);
		*out_start = (pmaaddrx_val - (*out_size >> 3) + 1) << 2;
		return 0;
	}

	sbi_printf_highlight("ERROR %s(): PMA%d is not NAPOT mode (enable)\n", __func__,
		   entry_id);
	*out_size = *out_start = -1;
	return SBI_EINVAL;
}

static bool has_pma_region_conflict(unsigned long start, unsigned long size, int *conflict_id)
{
	unsigned long _start, _size, _end, end;

	end = start + size - 1;

	for (int i = 0; i < ANDES_MAX_PMA_REGIONS; i++) {
		if (pmaxcfg_etyp(i) == PMACFG_ETYP_OFF)
			continue;

		/* Decode start address and its size from pmaaddrx */
		if (pmaaddrx_to_region(i, &_start, &_size))
			return true;

		_end = _start + _size - 1;

		if (max(start, _start) <= min(end, _end)) {
			sbi_printf_highlight("ERROR %s(): desired region %#lx - %#lx (size: %#lx) conflicts with the existing PMA%d: %#lx - %#lx (size: %#lx)\n",
					__func__, start, end, size, i, _start, _end, _size);
			*conflict_id = i;
			return true;
		}
	}

	return false;
}

int mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size)
{
	char *pmaxcfg;
	int rc, pma_cfg, conflict_id = -1, entry_id;
	unsigned long pmaaddr_val, pmacfg_val;

	/* Sanity check */
	if (!mcall_probe_pma()) {
		sbi_printf_highlight("ERROR %s(): platform does not support PPMA.\n",
			  __func__);
		goto fail;
	}

	/* Check size is 4KiB granularity and is power of 2 */
	if (size < ANDES_PMA_GRANULARITY || not_napot(pa, size)) {
		sbi_printf_highlight("ERROR %s(): %#lx - %#lx (size: %#lx) is not a 4KiB granularity NAPOT region\n",
			  __func__, pa, pa + size - 1, size);
		goto fail;
	}

	/* Not allow region overlapping */
	if (has_pma_region_conflict(pa, size, &conflict_id)) {
		sbi_printf_highlight(
			"ERROR %s(): PMA region overlaps with PMA%d used by va=%#lx\n",
			__func__, conflict_id,
			get_pma_table(conflict_id));
		goto fail;
	}

	/* Encode the start address and size */
	pmaaddr_val = (pa >> 2) + (size >> 3) - 1;

	rc = allocate_pma_entry(va, &entry_id);
	if (rc)
		return SBI_ENOENT;

#if __riscv_xlen == 64
	pma_cfg	   = CSR_PMACFG0 + ((entry_id / 8) ? 2 : 0);
	pmacfg_val = andes_pma_read_num(pma_cfg);
	pmaxcfg	   = (char *)&pmacfg_val + (entry_id % 8);
	*pmaxcfg   = 0;
	*pmaxcfg |= PMACFG_ETYP_NAPOT;
	*pmaxcfg |= PMACFG_MTYP_NOCACHE_BUFFER;
#else
	pma_cfg	     = CSR_PMACFG0 + entry_id / 4;
	pmacfg_val   = andes_pma_read_num(pma_cfg);
	pmaxcfg	     = (char *)&pmacfg_val + (entry_id % 4);
	*pmaxcfg     = 0;
	*pmaxcfg |= PMACFG_ETYP_NAPOT;
	*pmaxcfg |= PMACFG_MTYP_NOCACHE_BUFFER;
#endif

	/* Set pmaxcfg and pmaaddrx */
	andes_pma_write_num(pma_cfg, pmacfg_val);
	andes_pma_write_num(CSR_PMAADDR0 + entry_id, pmaaddr_val);

	if (andes_pma_read_num(CSR_PMAADDR0 + entry_id) != pmaaddr_val) {
		sbi_printf_highlight(
			"ERROR %s(): Failed to set the pmaaddr%d to desired value\n",
			__func__, entry_id);
		return SBI_EFAIL;
	}

	return 0;

fail:
	return SBI_EINVAL;
}

int mcall_free_pma(unsigned long va)
{
	unsigned long pmacfg_val;
	int pma_cfg;
	char *pmaxcfg;

	for (int i = 0; i < ANDES_MAX_PMA_REGIONS; i++) {
		if (get_pma_table(i) != va)
			continue;

		if (pmaxcfg_etyp(i) == PMACFG_ETYP_OFF)
			sbi_panic(
				"ERROR %s(): expecting PMA%d is enabled for %#lx\n",
				__func__, i, va);

		/* Free $pmacfg */
#if __riscv_xlen == 64
		pma_cfg	   = CSR_PMACFG0 + ((i / 8) ? 2 : 0);
		pmacfg_val = andes_pma_read_num(pma_cfg);
		pmaxcfg	   = (char *)&pmacfg_val + (i % 8);
		*pmaxcfg   = PMACFG_ETYP_OFF;
#else
		pma_cfg	   = CSR_PMACFG0 + i / 4;
		pmacfg_val = andes_pma_read_num(pma_cfg);
		pmaxcfg	   = (char *)&pmacfg_val + (i % 4);
		*pmaxcfg   = PMACFG_ETYP_OFF;
#endif
		andes_pma_write_num(pma_cfg, pmacfg_val);

		/* Free $pmaaddrx */
		andes_pma_write_num(CSR_PMAADDR0 + i, 0x0);

		/* Free the entry */
		set_pma_table(i, 0x0);

		return 0;
	}

	return 0;
}

int pma_init(void)
{
	/*
	 * Allocate PMA mapping table in every hart's scratch region
	 */
	pma_features_offset = sbi_scratch_alloc_offset(
					sizeof(struct andes_pma_data));
	if (!pma_features_offset)
		return SBI_ENOMEM;

	return SBI_OK;
}
