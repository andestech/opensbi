#ifndef _RISCV_ANDES45_H
#define _RISCV_ANDES45_H

/* clang-format off */

#define CSR_MARCHID_MICROID 0xfff

/* Memory and Miscellaneous Registers */
#define CSR_MPFT_CTL     0x7c5
#define CSR_MCACHE_CTL   0x7ca
#define CSR_MCCTLCOMMAND 0x7cc

/* Supervisor Trap Related Registers */
#define CSR_SLIP 0x9c5

#define IRQ_M_PMU 18
#define CSR_SLIP_PMOVI_MASK (1 << IRQ_M_PMU)
#define CSR_MIE_PMOVI_MASK  (1 << IRQ_M_PMU)

/* Configuration Control & Status Registers */
#define CSR_MICM_CFG  0xfc0
#define CSR_MDCM_CFG  0xfc1
#define CSR_MMSC_CFG  0xfc2
#define CSR_MMSC_CFG_PPMA_MASK (1 << 30)

/* clang-format on */

#endif /* _RISCV_ANDES45_H */
