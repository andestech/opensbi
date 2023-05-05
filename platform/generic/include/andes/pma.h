/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *	Nick Hu <nickhu@andestech.com>
 *	Nylon Chen <nylon7@andestech.com>
 *	Yu Chien Peter Lin <peterlin@andestech.com>
 */

#ifndef _ANDES_PMA_H
#define _ANDES_PMA_H

#include <sbi/sbi_error.h>

#define CSR_PMAADDR0 0xBD0
#define CSR_PMACFG0 0xBC0

#define PMA_ENTRY_NR 16
#define PMACFG_ETPY_MASK 0x3
#define PMACFG_MTPY_MASK (0xf << 2)
#define PMACFG_ETYP_OFF 0x0
#define PMACFG_ETYP_NAPOT 0x3
#define PMACFG_MTYP_NOCACHE_BUFFER 0x3

#ifndef __ASSEMBLER__

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#ifdef CONFIG_ANDES_PMA

struct andes_pma_data {
    /*
    * pma_user_table[] keeps track of which PMA entry is being used,
    * 0 indicates that it is unused and can be allocated.
    */
    virtual_addr_t pma_user_table[PMA_ENTRY_NR];
};

/**
 * Check if hardware support PPMA
 * @return true if PPMA is supported
 * @return false if PPMA is not supported
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
 * @return 0 on success
 * @return SBI_ENOENT if run out of the available entries
 * @return SBI_EFAIL otherwise
 */
int mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size);

/**
 * Free a PMA entry
 * @param va virtual address, the associated PMA entry of the va will be freed
 *
 * @return 0 whether on success or fail
 */
int mcall_free_pma(unsigned long va);

/**
 * Allocate PMA mapping table in every hart's scratch region
 *
 * @return SBI_OK on success
 * @return SBI_ENOMEM if scratch region is full
 */
int pma_init(void);

#else

static inline bool mcall_probe_pma(void) {
	return false;
}
static inline int mcall_set_pma(unsigned long pa, unsigned long va, unsigned long size) {
	return SBI_ENOTSUPP;
}
static inline int mcall_free_pma(unsigned long va) {
	return SBI_ENOTSUPP;
}
static inline int pma_init(void) {
	return SBI_ENOTSUPP;
}

#endif /* CONFIG_ANDES_PMA */

#endif /* __ASSEMBLER__ */

#endif /* _ANDES_PMA_H */
