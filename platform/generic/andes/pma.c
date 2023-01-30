/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Andes Technology Corporation
 *
 * Authors:
 *	Nick Hu <nickhu@andestech.com>
 *	Nylon Chen <nylon7@andestech.com>
 *	Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_types.h>
#include <andes/andes45.h>
#include <andes/pma.h>

/*
 * pma_user_table[] keeps track of which PMA entry is being used,
 * 0 indicates that it is unused and can be allocated.
 */
unsigned long pma_user_table[PMA_ENTRY_NR] = { 0 };

static void andes_pma_write_num(int csr_num, unsigned long val)
{
#define switchcase_csr_write(__csr_num, __val) \
	case __csr_num:                        \
		csr_write(__csr_num, __val);   \
		break;
#define switchcase_csr_write_2(__csr_num, __val)   \
	switchcase_csr_write(__csr_num + 0, __val) \
		switchcase_csr_write(__csr_num + 1, __val)
#define switchcase_csr_write_4(__csr_num, __val)     \
	switchcase_csr_write_2(__csr_num + 0, __val) \
		switchcase_csr_write_2(__csr_num + 2, __val)
#define switchcase_csr_write_8(__csr_num, __val)     \
	switchcase_csr_write_4(__csr_num + 0, __val) \
		switchcase_csr_write_4(__csr_num + 4, __val)
#define switchcase_csr_write_16(__csr_num, __val)    \
	switchcase_csr_write_8(__csr_num + 0, __val) \
		switchcase_csr_write_8(__csr_num + 8, __val)

	switch (csr_num) {
	switchcase_csr_write_4(CSR_PMACFG0, val)
	switchcase_csr_write_16(CSR_PMAADDR0, val)
	default:
		sbi_panic("%s: Unknown Andes CSR %#x", __func__, csr_num);
	break;
	};

#undef switchcase_csr_write_16
#undef switchcase_csr_write_8
#undef switchcase_csr_write_4
#undef switchcase_csr_write_2
#undef switchcase_csr_write
}

static unsigned long andes_pma_read_num(int csr_num)
{
#define switchcase_csr_read(__csr_num, __val) \
	case __csr_num:                       \
		__val = csr_read(__csr_num);  \
		break;
#define switchcase_csr_read_2(__csr_num, __val)   \
	switchcase_csr_read(__csr_num + 0, __val) \
		switchcase_csr_read(__csr_num + 1, __val)
#define switchcase_csr_read_4(__csr_num, __val)     \
	switchcase_csr_read_2(__csr_num + 0, __val) \
		switchcase_csr_read_2(__csr_num + 2, __val)
#define switchcase_csr_read_8(__csr_num, __val)     \
	switchcase_csr_read_4(__csr_num + 0, __val) \
		switchcase_csr_read_4(__csr_num + 4, __val)
#define switchcase_csr_read_16(__csr_num, __val)    \
	switchcase_csr_read_8(__csr_num + 0, __val) \
		switchcase_csr_read_8(__csr_num + 8, __val)
#define switchcase_csr_read_32(__csr_num, __val)     \
	switchcase_csr_read_16(__csr_num + 0, __val) \
		switchcase_csr_read_16(__csr_num + 16, __val)

	unsigned long ret = 0;

	switch (csr_num) {
	switchcase_csr_read_4(CSR_PMACFG0, ret)
	switchcase_csr_read_16(CSR_PMAADDR0, ret)
	default:
		sbi_panic("%s: Unknown Andes CSR %#x", __func__, csr_num);
	break;
	};

	return ret;

#undef switchcase_csr_read_16
#undef switchcase_csr_read_8
#undef switchcase_csr_read_4
#undef switchcase_csr_read_2
#undef switchcase_csr_read
}

static unsigned long read_pmaaddrx(int entry_id)
{
	return andes_pma_read_num(CSR_PMAADDR0 + entry_id);
}

static char read_pmaxcfg(int entry_id)
{
	unsigned long pmacfg_val;
	unsigned long pmaxcfg_mask;

	if (entry_id < 0 || entry_id > 15) {
		sbi_panic("ERROR %s(): non-existing PMA entry_id: %d\n",
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
	for (int i = 0; i < PMA_ENTRY_NR; i++) {
		if (pma_user_table[i] != va)
			continue;
		sbi_printf("ERROR %s(): va %#lx conflicts\n", __func__,
			  va);
		return true;
	}
	return false;
}

static u8 pmaxcfg_etyp(int entry_id)
{
	/* pmaxcfg.ETYP will be 0 when it is set to 1 or 2 (reserved) */
	u8 pmaxcfg_val = read_pmaxcfg(entry_id);
	return EXTRACT_FIELD(pmaxcfg_val, PMACFG_ETPY_MASK);
}

static int allocate_pma_entry(unsigned long va, int *entry_id)
{
	/* Not allow va alias */
	if(is_va_alias(va))
		goto fail;

	for (int i = 0; i < PMA_ENTRY_NR; i++) {
		if (pmaxcfg_etyp(i) != PMACFG_ETYP_OFF)
			continue;
		pma_user_table[i] = va;
		*entry_id	  = i;
		return SBI_SUCCESS;
	}

fail:
	sbi_printf("ERROR %s(): Cannot allocate PMA entry for %#lx\n", __func__,
		   va);
	*entry_id = -1;

	return SBI_ENOENT;
}

static inline bool is_napot(unsigned long base, unsigned long size)
{
	return !((size & (size - 1)) || base & (size - 1));
}

bool mcall_probe_pma(void)
{
	return !!EXTRACT_FIELD(csr_read(CSR_MMSC_CFG), CSR_MMSC_CFG_PPMA_MASK);
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

	sbi_printf("ERROR %s(): PMA%d is not NAPOT mode (enable)\n", __func__,
		   entry_id);
	*out_size = *out_start = -1;
	return SBI_EINVAL;
}

static bool has_pma_region_conflict(unsigned long start, unsigned long size, int *conflict_id)
{
	unsigned long _start, _size, _end, end;

	end = start + size - 1;

	for (int i = 0; i < PMA_ENTRY_NR; i++) {
		if (pmaxcfg_etyp(i) == PMACFG_ETYP_OFF)
			continue;

		/* Decode start address and its size from pmaaddrx */
		if (pmaaddrx_to_region(i, &_start, &_size))
			return true;

		_end = _start + _size - 1;

		if (max(start, _start) <= min(end, _end)) {
			sbi_printf("ERROR %s(): desired region %#lx - %#lx (size: %#lx) conflicts with the existing PMA%d: %#lx - %#lx (size: %#lx)\n",
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
		sbi_printf("ERROR %s(): platform does not support PPMA.\n",
			  __func__);
		goto fail;
	}

	if (size < 4096 || !is_napot(pa, size)) {
		sbi_printf("ERROR %s(): %#lx - %#lx (size: %#lx) is not a 4KiB granularity NAPOT region\n",
			  __func__, pa, pa + size - 1, size);
		goto fail;
	}

	/* Not allow region overlapping */
	if (has_pma_region_conflict(pa, size, &conflict_id)) {
		sbi_printf(
			"ERROR %s(): PMA region overlaps with PMA%d used by va=%#lx\n",
			__func__, conflict_id,
			pma_user_table[conflict_id]);
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
		sbi_printf(
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

	for (int i = 0; i < PMA_ENTRY_NR; i++) {
		if (pma_user_table[i] != va)
			continue;

		if (pmaxcfg_etyp(i) == PMACFG_ETYP_OFF)
			sbi_panic(
				"ERROR %s(): expecting PMA%d is enabled for %#lx\n",
				__func__, i, va);

		/* Free the entry */
		pma_user_table[i] = 0x0;
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
		return 0;
	}

	sbi_printf("ERROR %s(): failed to free the PMA entry used by %#lx\n",
		   __func__, va);

	return SBI_ENOENT;
}
