/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 *
 * Authors:
 *   Yu Chien Peter Lin <peterlin@andestech.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/riscv_asm.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/sys/atcsmu.h>
#include <andes/andesv5.h>

	.section .text, "ax", %progbits
	.align 3
	.global __ae350_disable_coherency
__ae350_disable_coherency:
	/* flush d-cache */
	csrw	CSR_MCCTLCOMMAND, 0x6
	/* disable i/d-cache */
	csrc	CSR_MCACHE_CTL, 0x3
	/* disable d-cache coherency */
	lui	t1, 0x80
	csrc	CSR_MCACHE_CTL, t1
	/*
	 * wait for mcache_ctl.DC_COHSTA to be cleared,
	 * the bit is hard-wired 0 on platforms w/o CM
	 * (Coherence Manager)
	 */
check_cm_disabled:
	csrr	t1, CSR_MCACHE_CTL
	srli	t1, t1, 20
	andi	t1, t1, 0x1
	bnez	t1, check_cm_disabled

	ret

	.section .text, "ax", %progbits
	.align 3
	.global __ae350_enable_coherency
__ae350_enable_coherency:
	/* enable d-cache coherency */
	lui		t1, 0x80
	csrs	CSR_MCACHE_CTL, t1
	/*
	 * mcache_ctl.DC_COHEN is hard-wired 0 on platforms
	 * w/o CM support
	 */
	csrr	t1, CSR_MCACHE_CTL
	srli	t1, t1, 19
	andi	t1, t1, 0x1
	beqz	t1, enable_L1_cache
	/* wait for mcache_ctl.DC_COHSTA to be set */
check_cm_enabled:
	csrr	t1, CSR_MCACHE_CTL
	srli	t1, t1, 20
	andi	t1, t1, 0x1
	beqz	t1, check_cm_enabled
enable_L1_cache:
	/* enable i/d-cache */
	csrs	CSR_MCACHE_CTL, 0x3

	ret

	.section .text, "ax", %progbits
	.align 3
	.global __ae350_enable_coherency_warmboot
__ae350_enable_coherency_warmboot:
	call ra, __ae350_enable_coherency
	j _start_warm


sleep_hang:
	j sleep_hang

.macro SAVE_ALL_REGS
	/* Store x1 ~ x31 to stack */
	PUSH(x1)
	PUSH(x2)
	PUSH(x3)
	PUSH(x4)
	PUSH(x5)
	PUSH(x6)
	PUSH(x7)
	PUSH(x8)
	PUSH(x9)
	PUSH(x10)
	PUSH(x11)
	PUSH(x12)
	PUSH(x13)
	PUSH(x14)
	PUSH(x15)
	PUSH(x16)
	PUSH(x17)
	PUSH(x18)
	PUSH(x19)
	PUSH(x20)
	PUSH(x21)
	PUSH(x22)
	PUSH(x23)
	PUSH(x24)
	PUSH(x25)
	PUSH(x26)
	PUSH(x27)
	PUSH(x28)
	PUSH(x29)
	PUSH(x30)
	PUSH(x31)

	/* Push RISC-V m-mode reg */
	PUSH_CSR(CSR_MSTATUS)
	PUSH_CSR(CSR_MEDELEG)
	PUSH_CSR(CSR_MIDELEG)
	PUSH_CSR(CSR_MIE)
	PUSH_CSR(CSR_MTVEC)
	PUSH_CSR(CSR_MSCRATCH)
	PUSH_CSR(CSR_MCAUSE)
	PUSH_CSR(CSR_MTVAL)
	PUSH_CSR(CSR_MIP)
	PUSH_CSR(CSR_MCOUNTEREN)
	PUSH_CSR(CSR_MCOUNTINHIBIT)

	/* Push Andes m-mode reg */
	PUSH_CSR(CSR_MHSP_CTL)
	PUSH_CSR(CSR_MSP_BOUND)
	PUSH_CSR(CSR_MSP_BASE)
	PUSH_CSR(CSR_MXSTATUS)
	PUSH_CSR(CSR_MDCAUSE)
	PUSH_CSR(CSR_MSLIDELEG)
	PUSH_CSR(CSR_MPFT_CTL)
	PUSH_CSR(CSR_MMISC_CTL)
	PUSH_CSR(CSR_MCACHE_CTL)

	/* Check if PPMA exists */
	csrr	t0, CSR_MMSC_CFG
	li	t1, PPMA_EN
	and	t0, t0, t1
	beqz	t0, 1f
#if __riscv_xlen == 64
	PUSH_CSR(CSR_PMACFG0)
	PUSH_CSR(CSR_PMACFG2)
#else
	PUSH_CSR(CSR_PMACFG0)
	PUSH_CSR(CSR_PMACFG1)
	PUSH_CSR(CSR_PMACFG2)
	PUSH_CSR(CSR_PMACFG3)
#endif
	PUSH_CSR(CSR_PMAADDR0)
	PUSH_CSR(CSR_PMAADDR1)
	PUSH_CSR(CSR_PMAADDR2)
	PUSH_CSR(CSR_PMAADDR3)
	PUSH_CSR(CSR_PMAADDR4)
	PUSH_CSR(CSR_PMAADDR5)
	PUSH_CSR(CSR_PMAADDR6)
	PUSH_CSR(CSR_PMAADDR7)
	PUSH_CSR(CSR_PMAADDR8)
	PUSH_CSR(CSR_PMAADDR9)
	PUSH_CSR(CSR_PMAADDR10)
	PUSH_CSR(CSR_PMAADDR11)
	PUSH_CSR(CSR_PMAADDR12)
	PUSH_CSR(CSR_PMAADDR13)
	PUSH_CSR(CSR_PMAADDR14)
	PUSH_CSR(CSR_PMAADDR15)

1:	/* Push PMP */
#if __riscv_xlen == 64
	PUSH_CSR(CSR_PMPCFG0)
	PUSH_CSR(CSR_PMPCFG2)
#else
	PUSH_CSR(CSR_PMPCFG0)
	PUSH_CSR(CSR_PMPCFG1)
	PUSH_CSR(CSR_PMPCFG2)
	PUSH_CSR(CSR_PMPCFG3)
#endif
	PUSH_CSR(CSR_PMPADDR0)
	PUSH_CSR(CSR_PMPADDR1)
	PUSH_CSR(CSR_PMPADDR2)
	PUSH_CSR(CSR_PMPADDR3)
	PUSH_CSR(CSR_PMPADDR4)
	PUSH_CSR(CSR_PMPADDR5)
	PUSH_CSR(CSR_PMPADDR6)
	PUSH_CSR(CSR_PMPADDR7)
	PUSH_CSR(CSR_PMPADDR8)
	PUSH_CSR(CSR_PMPADDR9)
	PUSH_CSR(CSR_PMPADDR10)
	PUSH_CSR(CSR_PMPADDR11)
	PUSH_CSR(CSR_PMPADDR12)
	PUSH_CSR(CSR_PMPADDR13)
	PUSH_CSR(CSR_PMPADDR14)
	PUSH_CSR(CSR_PMPADDR15)

	/* Push RISC-V s-mode reg */
	PUSH_CSR(CSR_SSTATUS)
	PUSH_CSR(CSR_SIE)
	PUSH_CSR(CSR_STVEC)
	PUSH_CSR(CSR_SCOUNTEREN)
	PUSH_CSR(CSR_SSCRATCH)
	PUSH_CSR(CSR_SEPC)
	PUSH_CSR(CSR_SCAUSE)
	PUSH_CSR(CSR_STVAL)
	PUSH_CSR(CSR_SIP)
	PUSH_CSR(CSR_SATP)

	/* Push Andes s-mode reg */
	PUSH_CSR(CSR_SLIE)
	PUSH_CSR(CSR_SLIP)
	PUSH_CSR(CSR_SDCAUSE)
.endm

.macro RESTORE_ALL_REGS
	/* Pop Andes s-mode reg */
	POP_CSR(CSR_SDCAUSE)
	POP_CSR(CSR_SLIP)
	POP_CSR(CSR_SLIE)

	/* Pop RISC-V s-mode reg */
	sfence.vma
	POP_CSR(CSR_SATP)
	sfence.vma

	POP_CSR(CSR_SIP)
	POP_CSR(CSR_STVAL)
	POP_CSR(CSR_SCAUSE)
	POP_CSR(CSR_SEPC)
	POP_CSR(CSR_SSCRATCH)
	POP_CSR(CSR_SCOUNTEREN)
	POP_CSR(CSR_STVEC)
	POP_CSR(CSR_SIE)
	POP_CSR(CSR_SSTATUS)

	/* Pop PMP */
	POP_CSR(CSR_PMPADDR15)
	POP_CSR(CSR_PMPADDR14)
	POP_CSR(CSR_PMPADDR13)
	POP_CSR(CSR_PMPADDR12)
	POP_CSR(CSR_PMPADDR11)
	POP_CSR(CSR_PMPADDR10)
	POP_CSR(CSR_PMPADDR9)
	POP_CSR(CSR_PMPADDR8)
	POP_CSR(CSR_PMPADDR7)
	POP_CSR(CSR_PMPADDR6)
	POP_CSR(CSR_PMPADDR5)
	POP_CSR(CSR_PMPADDR4)
	POP_CSR(CSR_PMPADDR3)
	POP_CSR(CSR_PMPADDR2)
	POP_CSR(CSR_PMPADDR1)
	POP_CSR(CSR_PMPADDR0)
#if __riscv_xlen == 64
	POP_CSR(CSR_PMPCFG2)
	POP_CSR(CSR_PMPCFG0)
#else
	POP_CSR(CSR_PMPCFG3)
	POP_CSR(CSR_PMPCFG2)
	POP_CSR(CSR_PMPCFG1)
	POP_CSR(CSR_PMPCFG0)
#endif

	/* Check if PPMA exists */
	csrr	t0, CSR_MMSC_CFG
	li	t1, PPMA_EN
	and	t0, t0, t1
	beqz	t0, 2f
	/* Pop Andes m-mode reg */
	POP_CSR(CSR_PMAADDR15)
	POP_CSR(CSR_PMAADDR14)
	POP_CSR(CSR_PMAADDR13)
	POP_CSR(CSR_PMAADDR12)
	POP_CSR(CSR_PMAADDR11)
	POP_CSR(CSR_PMAADDR10)
	POP_CSR(CSR_PMAADDR9)
	POP_CSR(CSR_PMAADDR8)
	POP_CSR(CSR_PMAADDR7)
	POP_CSR(CSR_PMAADDR6)
	POP_CSR(CSR_PMAADDR5)
	POP_CSR(CSR_PMAADDR4)
	POP_CSR(CSR_PMAADDR3)
	POP_CSR(CSR_PMAADDR2)
	POP_CSR(CSR_PMAADDR1)
	POP_CSR(CSR_PMAADDR0)
#if __riscv_xlen == 64
	POP_CSR(CSR_PMACFG2)
	POP_CSR(CSR_PMACFG0)
#else
	POP_CSR(CSR_PMACFG3)
	POP_CSR(CSR_PMACFG2)
	POP_CSR(CSR_PMACFG1)
	POP_CSR(CSR_PMACFG0)
#endif

	/* Pop cache setting */
2:	POP_CSR(CSR_MCACHE_CTL)

	POP_CSR(CSR_MMISC_CTL)
	POP_CSR(CSR_MPFT_CTL)
	POP_CSR(CSR_MSLIDELEG)
	POP_CSR(CSR_MDCAUSE)
	POP_CSR(CSR_MXSTATUS)
	POP_CSR(CSR_MSP_BASE)
	POP_CSR(CSR_MSP_BOUND)
	POP_CSR(CSR_MHSP_CTL)

	/* Pop RISC-V m-mode reg */
	POP_CSR(CSR_MCOUNTINHIBIT)
	POP_CSR(CSR_MCOUNTEREN)
	POP_CSR(CSR_MIP)
	POP_CSR(CSR_MTVAL)
	POP_CSR(CSR_MCAUSE)
	POP_CSR(CSR_MSCRATCH)
	POP_CSR(CSR_MTVEC)
	POP_CSR(CSR_MIE)
	POP_CSR(CSR_MIDELEG)
	POP_CSR(CSR_MEDELEG)
	POP_CSR(CSR_MSTATUS)

	/* Pop x1 ~ x31 */
	POP(x31)
	POP(x30)
	POP(x29)
	POP(x28)
	POP(x27)
	POP(x26)
	POP(x25)
	POP(x24)
	POP(x23)
	POP(x22)
	POP(x21)
	POP(x20)
	POP(x19)
	POP(x18)
	POP(x17)
	POP(x16)
	POP(x15)
	POP(x14)
	POP(x13)
	POP(x12)
	POP(x11)
	POP(x10)
	POP(x9)
	POP(x8)
	POP(x7)
	POP(x6)
	POP(x5)
	POP(x4)
	POP(x3)
	POP(x2)
	POP(x1)

	ret
.endm

.macro SAVE_L2_SETTING l2_base_addr
	mv	t0, \l2_base_addr
	lw	t0, 8(t0)		/* l2c base + 0x8: l2c control register offset */
	PUSH(t0)			/* Store l2c setting */
.endm

.macro SET_RESET_VEC reset_vec
	li	t0, 0x4			/* 0x4: The offset of the reset vector register of each hart */
	csrr	t1, CSR_MHARTID
	mul	t0, t0, t1
	addi	t0, t0, 0x50		/* hart0: 0x50, hart1: 0x54, hart2: 0x58, hart3: 0x5c */

	/* 8-core */
	li	t1, 0x60		/* if (t1 = 0x50 + mhartid * 0x4) >= 0x60, then mhartid >= 0x4 */
	blt	t0, t1, 10f
	addi	t0, t0, 0x1a0		/* hart4: 0x200, hart5: 0x204, hart6: 0x208, hart7: 0x20c */
10:
	la	t1, smu
	REG_L	t1, 0(t1)		/* t1 = smu.addr */
	add	t0, t0, t1
	la	t1, \reset_vec
	sw	t1, 0(t0)

	/* Reset MIE */
	csrw	CSR_MIE, 0
	csrw	CSR_MSTATUS, 0
	csrw	CSR_MIP, 0

	/* Reset SIE */
	csrw	CSR_SIE, 0
	csrw	CSR_SSTATUS, 0
	csrw	CSR_SIP, 0
.endm

.macro DISABLE_I_D_CACHE
	/* Write Back Invalidate all d-cache */
	csrw	CSR_MCCTLCOMMAND, 0x6
	/* Disable d-cache & i-cache */
	csrc	CSR_MCACHE_CTL, 0x3
	/* Disable CM (DC_COHEN) */
	li	t1, 0x80000		/* DC_COHEN: 19th bit of mcache */
	csrc	CSR_MCACHE_CTL, t1

20:	/* Check if CM(DC_COHSTA) is set */
	csrr	t1, CSR_MCACHE_CTL
	srli	t1, t1, 20		/* DC_COHSTA: 20th bit of mcache_ctl */
	andi	t1, t1, 0x1
	bnez	t1, 20b
.endm

.macro DISABLE_L2_CACHE l2_base_addr
	/* Write back invalidate L2 */
	mv	t0, \l2_base_addr
	li	t2, 0x10		/* 0x10: CCTL command hart's offset */
	mul	t1, t1, t2
	add	t1, t1, 0x40		/* 0x40: CCTL command offset */
	add	t0, t0, t1
	li	t1, 0x12		/* 0x12: L2_WBINVAL_ALL command */
	sw	t1, 0(t0)

30:
	/* Polling L2 idle status for core0 */
	lw	t1, 0x80(\l2_base_addr)	/* 0x80: CCTL status register offset */
	andi	t1, t1, 0xf		/* 0xf: The first 4 bit is hart0 status register */
	bnez	t1, 30b

	/* Disable L2 */
	lw	t1, 0x8(\l2_base_addr)	/* 0x8: CCTL control register */
	srli	t1, t1, 1
	slli	t1, t1, 1
	sw	t1, 0x8(\l2_base_addr)
.endm

.macro EXEC_WFI
	/* Store $sp to pcs scratch for each core */
	li	t0, 0x20		/* 0x20: offset of each hart's scratch register */
	li	t1, 3			/* starting from PCS'3' */
	csrr	t2, CSR_MHARTID
	add	t1, t1, t2
	mul	t0, t0, t1
	addi	t0, t0, 0x84		/* scratch register: 0x84 + (3 + mhartid) * 0x20 */
	la	t1, smu
	REG_L	t1, 0(t1)		/* t1 = smu.addr */
	add	t0, t0, t1
	sw	sp, 0(t0)

	wfi

	j	sleep_hang
.endm

.macro RESTORE_SP
	/* Check if it comes from NMI, go resume if not */
	csrr	t0, CSR_MCAUSE
	beqz	t0, 40f
	j	sleep_hang

40:	/* Load $sp */
	li	t0, 0x20		/* 0x20: offset of each hart's scratch register */
	li	t1, 3			/* starting from PCS'3' */
	csrr	t2, CSR_MHARTID
	add	t1, t1, t2
	mul	t0, t0, t1
	addi	t0, t0, 0x84		/* scratch register: 0x84 + (3 + mhartid) * 0x20 */
	la	t1, smu
	REG_L	t1, 0(t1)		/* t1 = smu.addr */
	add	t0, t0, t1
	lw	sp, 0(t0)
.endm

.macro ENABLE_L2
	/* Allocate L2 setting */
	addi	sp, sp, -REGBYTES	/* Allocate memory from stack */
	mv	a0, sp			/* Use the allocated space as arg passed to cache_get_addr */
	call	cache_get_addr
	REG_L	t0, 0(sp)		/* Get addr from cache_get_addr to $t0 */
	addi	sp, sp, REGBYTES	/* Free the allocated space */
	bltz	a0, 60f

50:	/* Restore L2 setting */
	POP(t1)
	sw	t1, 0x8(t0)
60:
	nop
.endm

.macro ENABLE_I_D_CACHE
	/* Enable CM (DC_COHEN) */
	csrr	t1, CSR_MCACHE_CTL
	lui	t2, 0x80
	or	t1, t1, t2
	csrw	CSR_MCACHE_CTL, t1

	/* Check DC_COHEN is enabled (25-series does not have CM) */
	csrr	t1, CSR_MCACHE_CTL
	lui	t2, 0x80
	bne	t1, t2, 80f

70:
	csrr	t1, CSR_MCACHE_CTL
	srli	t1, t1, 12
	li	a5, 0x100
	and	t1, t1, a5
	beqz	t1, 70b

80:	/* Enable d-cache & i-cache */
	csrs	CSR_MCACHE_CTL, 0x3
.endm

	/* Hotplug */
	.section .text, "ax", %progbits
	.align 3
	.global suspend2ram_hotplug
suspend2ram_hotplug:
	SAVE_ALL_REGS
	SET_RESET_VEC cpu_resume_hotplug
	DISABLE_I_D_CACHE
	EXEC_WFI

	/* Resume from hotplug */
	.section .text, "ax", %progbits
	.align 3
	.global cpu_resume_hotplug
cpu_resume_hotplug:
	RESTORE_SP
	ENABLE_I_D_CACHE
	RESTORE_ALL_REGS
	ret

	/* Deep sleep */
	.section .text, "ax", %progbits
	.align 3
	.global suspend2ram_deep_sleep
suspend2ram_deep_sleep:
	mv	s0, a0			/* $a0 == l2c.addr */
	csrr	s1, CSR_MHARTID
	SAVE_ALL_REGS
	beqz	s0, _set_reset_vec	/* if (l2c.addr == 0) then omit "save l2 setting" */
	bnez	s1, _set_reset_vec	/* if (mhartid != 0) then omit "save l2 setting" */
	SAVE_L2_SETTING s0
_set_reset_vec:
	SET_RESET_VEC cpu_resume_deep_sleep
	DISABLE_I_D_CACHE
	beqz	s0, _sleep		/* if (l2c.addr == 0) then omit "disable L2" */
	bnez	s1, _sleep		/* if (mhartid != 0) then omit "disable L2" */
	DISABLE_L2_CACHE s0
_sleep:
	EXEC_WFI

	/* Resume from deep sleep */
	.section .text, "ax", %progbits
	.align 3
	.global cpu_resume_deep_sleep
cpu_resume_deep_sleep:
	RESTORE_SP
	csrr	s1, CSR_MHARTID
	bnez	s1, _enable_L1_cache
	ENABLE_L2
_enable_L1_cache:
	ENABLE_I_D_CACHE
	RESTORE_ALL_REGS
	ret
