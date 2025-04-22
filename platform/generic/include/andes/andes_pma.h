/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 Renesas Electronics Corp.
 */

#ifndef _ANDES_PMA_H_
#define _ANDES_PMA_H_

#include <sbi/sbi_error.h>
#include <sbi/sbi_types.h>

/* The supported amount of regions can be checked by pma_probe_ver() */
#define ANDES_MAX_PMA_REGIONS			48
/* Required in unit of 4KB */
#define ANDES_PMA_GRANULARITY			(1 << 12)

#define ANDES_PMACFG_ETYP_OFFSET		0
/* Bit[1:0] of PMAxCFG field in pmacfgX register */
#define ANDES_PMACFG_ETYP_MASK			(0x3 << ANDES_PMACFG_ETYP_OFFSET)
/* OFF: This PMA entry is disabled */
#define ANDES_PMACFG_ETYP_OFF			(0x0 << ANDES_PMACFG_ETYP_OFFSET)
/* NAPOT: Naturally aligned power of 2 region */
#define ANDES_PMACFG_ETYP_NAPOT			(0x3 << ANDES_PMACFG_ETYP_OFFSET)

#define ANDES_PMACFG_MTYP_OFFSET		2
/* Bit[5:2] of PMAxCFG field in pmacfgX register */
#define ANDES_PMACFG_MTYP_MASK			(0xf << ANDES_PMACFG_MTYP_OFFSET)
/* Memory, Non-cacheable, Bufferable */
#define ANDES_PMACFG_MTYP_MEM_NON_CACHE_BUF	(0x3 << ANDES_PMACFG_MTYP_OFFSET)

#define PPMA_VERSION_16_ENRTY			0
#define PPMA_VERSION_48_ENRTY			2

/**
 * struct andes_pma_region - Describes PMA regions
 *
 * @pa: Address to be configured in the PMA
 * @size: Size of the region
 * @flags: Flags to be set for the PMA region
 * @dt_populate: Boolean flag indicating if the DT entry should be
 *               populated for the given PMA region
 * @shared_dma: Boolean flag if set "shared-dma-pool" property will
 *              be set in the DT node
 * @no_map: Boolean flag if set "no-map" property will be set in the
 *          DT node
 * @dma_default: Boolean flag if set "linux,dma-default" property will
 *              be set in the DT node. Note Linux expects single node
 *              with this property set.
 */
struct andes_pma_region {
	unsigned long pa;
	unsigned long size;
	u8 flags:7;
	bool dt_populate;
	bool shared_dma;
	bool no_map;
	bool dma_default;
};

int andes_pma_setup_regions(const struct andes_pma_region *pma_regions,
			    unsigned int pma_regions_count);

#ifdef CONFIG_ANDES_PMA

struct andes_pma_data {
	/**
	 * pma_user_table[] keeps track of which PMA entry is being used,
	 * 0 indicates that it is unused and can be allocated.
	 */
	virtual_addr_t pma_user_table[ANDES_MAX_PMA_REGIONS];
};

/**
 * Check if hardware support PPMA
 * @return Boolean value indicating PPMA is supported or not
 */
bool mcall_probe_pma(void);

/**
 * Allocate a PMA entry for a "memory, non-cacheable, bufferable" NAPOT region
 * @param pa start address of NAPOT region
 * @param va virtual address to be stored in pma_user_table[i] which
 *           indicates the PMA entry has been allocated
 * @param size size of NAPOT region
 * @param entry_id (deprecated)
 *
 * @return Status code indicating the result
 * - SBI_OK: success
 * - SBI_ENOENT: run out of the available entries
 * - SBI_EFAIL: otherwise
 */
int mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size);

/**
 * Free a PMA entry
 * @param va virtual address, the associated PMA entry of the va will be freed
 *
 * @return Status code SBI_OK whether on success or fail
 */
int mcall_free_pma(unsigned long va);

/**
 * Detect PPMA verion
 *
 * Legal value:
 * 	0: 16 entry version -> PPMA_VERSION_16_ENRTY
 * 	2: 48 entry version -> PPMA_VERSION_48_ENRTY
 *
 * @return Version of the ANDES PPMA
 * - PPMA_VERSION_16_ENRTY: PPMA with max 16 entries available
 * - PPMA_VERSION_48_ENRTY: PPMA with max 48 entries available
 */
int pma_probe_ver(void);

/**
 * Allocate PMA mapping table in every hart's scratch region
 *
 * @return Status code indicating the result:
 * - SBI_OK: success
 * - SBI_ENOMEM: scratch region is full
 */
int pma_init(void);

#else

static inline bool mcall_probe_pma(void) { return false; }

static inline int
mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size)
{
	return SBI_ENOTSUPP;
}

static inline int mcall_free_pma(unsigned long va) { return SBI_ENOTSUPP; }

static inline int pma_probe_ver(void) { return SBI_ENOTSUPP; }

static inline int pma_init(void) { return SBI_ENOTSUPP; }

#endif /* CONFIG_ANDES_PMA */

#endif /* _ANDES_PMA_H_ */
