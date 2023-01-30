#ifndef _RISCV_ANDES45_H
#define _RISCV_ANDES45_H

#define CSR_MARCHID_MICROID 0xfff

/* Memory and Miscellaneous Registers */
#define CSR_MPFT_CTL 0x7c5
#define CSR_MCACHE_CTL 0x7ca
#define CSR_MCCTLCOMMAND 0x7cc

/* Supervisor Trap Related Registers */
#define CSR_SLIP 0x9c5

#define IRQ_M_PMU       18
#define CSR_SLIP_PMOVI_MASK (1 << IRQ_M_PMU)
#define CSR_MIE_PMOVI_MASK  (1 << IRQ_M_PMU)

#endif /* _RISCV_ANDES45_H */
