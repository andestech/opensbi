// See LICENSE for license details.

#ifndef _RISCV_SMU_H
#define _RISCV_SMU_H


#if __riscv_xlen == 64
# define SLL32    sllw
# define STORE    sd
# define LOAD     ld
# define LWU      lwu
# define LOG_REGBYTES 3
#else
# define SLL32    sll
# define STORE    sw
# define LOAD     lw
# define LWU      lw
# define LOG_REGBYTES 2
#endif
#define REGBYTES (1 << LOG_REGBYTES)
#define PUSH(which) addi sp, sp, -REGBYTES; STORE which, 0(sp)
#define PUSH_CSR(csr) addi sp, sp, -REGBYTES; csrr t0, csr; STORE t0, 0(sp)
#define POP(which)  LOAD which, 0(sp); addi sp, sp, REGBYTES
#define POP_CSR(csr)  LOAD t0, 0(sp); addi sp, sp, REGBYTES; csrw csr, t0

#define DRAM_BASE		0x80000000
#define SMU_BASE		0xf0100000
#define PCS0_CTL_OFF    	0x94
#define SMU_RESET_VEC_OFF       0x50
#define SMU_RESET_VEC_PER_CORE  0x4
#define RESET_CMD               0x1

#endif
