#include <sbi/riscv_encoding.h>
#include <sbi/riscv_asm.h>
#include "smu.h"
#include "platform.h"
#include "pma.h"

	.section .text, "ax", %progbits
	.align 3
	.global cpu_suspend2ram
cpu_suspend2ram:

	# backup cpu register
	# store x1 ~ x31 to stack
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

	# Push RISC-V m-mode reg
	PUSH_CSR(CSR_MSTATUS)
	PUSH_CSR(CSR_MISA)
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

	# Push Andes m-mode reg
	PUSH_CSR(CSR_MHSP_CTL)
	PUSH_CSR(CSR_MSP_BOUND)
	PUSH_CSR(CSR_MSP_BASE)
	PUSH_CSR(CSR_MXSTATUS)
	PUSH_CSR(CSR_MDCAUSE)
	PUSH_CSR(CSR_MSLIDELEG)
	PUSH_CSR(CSR_MPFT_CTL)
	PUSH_CSR(CSR_MMISC_CTL)
	PUSH_CSR(CSR_MCOUNTERWEN)
	PUSH_CSR(CSR_MCOUNTERINTEN)
	PUSH_CSR(CSR_MCOUNTERMASK_M)
	PUSH_CSR(CSR_MCOUNTERMASK_S)
	PUSH_CSR(CSR_MCOUNTERMASK_U)
	PUSH_CSR(CSR_MCOUNTEROVF)
	# Push cache setting
	PUSH_CSR(CSR_MCACHE_CTL)

	# check if PPMA exists
	csrr	t0, CSR_MMSC_CFG
	li	t1, 0x40000000	# 30th bit: PPMA
	and	t0, t0, t1
	beqz	t0, 1f
#if __riscv_xlen == 64
	PUSH_CSR(PMACFG_0)
	PUSH_CSR(PMACFG_2)
#else
	PUSH_CSR(PMACFG_0)
	PUSH_CSR(PMACFG_1)
	PUSH_CSR(PMACFG_2)
	PUSH_CSR(PMACFG_3)
#endif
	PUSH_CSR(PMAADDR_0)
	PUSH_CSR(PMAADDR_1)
	PUSH_CSR(PMAADDR_2)
	PUSH_CSR(PMAADDR_3)
	PUSH_CSR(PMAADDR_4)
	PUSH_CSR(PMAADDR_5)
	PUSH_CSR(PMAADDR_6)
	PUSH_CSR(PMAADDR_7)
	PUSH_CSR(PMAADDR_8)
	PUSH_CSR(PMAADDR_9)
	PUSH_CSR(PMAADDR_10)
	PUSH_CSR(PMAADDR_11)
	PUSH_CSR(PMAADDR_12)
	PUSH_CSR(PMAADDR_13)
	PUSH_CSR(PMAADDR_14)
	PUSH_CSR(PMAADDR_15)

	# Push RISC-V s-mode reg
1:	PUSH_CSR(CSR_SSTATUS)
	PUSH_CSR(CSR_SIE)
	PUSH_CSR(CSR_STVEC)
	PUSH_CSR(CSR_SCOUNTEREN)
	PUSH_CSR(CSR_SSCRATCH)
	PUSH_CSR(CSR_SEPC)
	PUSH_CSR(CSR_SCAUSE)
	PUSH_CSR(CSR_STVAL)
	PUSH_CSR(CSR_SIP)
	PUSH_CSR(CSR_SATP)

	# Push Andes s-mode reg
	PUSH_CSR(CSR_SLIE)
	PUSH_CSR(CSR_SLIP)
	PUSH_CSR(CSR_SDCAUSE)
	PUSH_CSR(CSR_SCOUNTERINTEN)
	PUSH_CSR(CSR_SCOUNTERMASK_M)
	PUSH_CSR(CSR_SCOUNTERMASK_S)
	PUSH_CSR(CSR_SCOUNTERMASK_U)
	PUSH_CSR(CSR_SCOUNTEROVF)
	PUSH_CSR(CSR_SCOUNTINHIBIT)

	# Push pmp
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

	# Store L2 setting before disabling cache
	# Only store it on hart 0's stack
	csrr	t1, CSR_MHARTID
	bnez	t1, store_rst_vec
	# Check if L2 exists
	la	t0, l2c
	REG_L	t0, 0(t0)
	beqz	t0, store_rst_vec
	# Save L2 setting
	lw	t0, 8(t0)	# l2c base + 0x8: l2c control register offset
	PUSH(t0)

	# store reset vector
store_rst_vec:
	li	t0, 0x4
	csrr	t1, CSR_MHARTID
	mul	t0, t0, t1
	addi	t0, t0, 0x50
eight_core:
	li	t1, 0x60
	blt	t0, t1, set_rst_vec
	addi	t0, t0, 0x1a0
set_rst_vec:
	la	t1, smu
	REG_L	t1, 0(t1)
	add	t0, t0, t1
	la	t1, cpu_resume
	sw	t1, 0(t0)

	#reset MIE
	csrw CSR_MIE, 0
	csrw CSR_MSTATUS, 0
	csrw CSR_MIP, 0

	# reset SIE
	csrw CSR_SIE, 0
	csrw CSR_SSTATUS, 0
	csrw CSR_SIP, 0

	# need to get ae350_suspend_mode[n] to $a4 before CM is disabled
	# $a4 = *(&ae350_suspend_mode + (mhartid * sizeof(int)))
	csrr  t1, CSR_MHARTID
	slli  t1, t1, 0x2
	la	  t0, ae350_suspend_mode
	add	  t0, t0, t1
	lw	  a4, 0(t0)

	# read l2c address parsed from fdt before CM is disabled
	# $t4 = l2c.addr
	la t4, l2c
	REG_L t4, 0(t4)

disable_I_D_cache:
	# flush dcache
	csrw	CSR_UCCTLCOMMAND, 0x6

	# disable d-cache & i-cache
	csrrc	t0, CSR_MCACHE_CTL, 0x2
	csrrc	t0, CSR_MCACHE_CTL, 0x1

disable_CM:
	# disable CM (DC_COHEN)
	csrr  t1, CSR_MCACHE_CTL
	lui   t2, 0xfff80
	addi  t2, t2, -1
	and   t1, t1, t2
	csrw  CSR_MCACHE_CTL, t1

wait_for_DC_COHSTA_disable:
	csrr   t1, CSR_MCACHE_CTL
	srli   t1, t1, 12
	li     a5, 0x100
	and    t1, t1, a5
	bnez   t1, wait_for_DC_COHSTA_disable

suspend_mode_check1:
	# ae350_suspend_mode[n] == DeepSleepMode --> goto disable_L2()
	li t1, DeepSleepMode
	beq a4, t1, disable_L2

	# ae350_suspend_mode[n] == CpuHotplugDeepSleepMode --> skip disable_L2()
	li t1, CpuHotplugDeepSleepMode
	beq a4, t1, goto_sleep

	j sleep_hang

disable_L2:
	# check if l2 exist
	beq x0, t4, goto_sleep

	# check if it is core 0
	csrr	t1, CSR_MHARTID
	bnez	t1, goto_sleep

	mv	t0, t4
	li	t2, 0x10
	mul	t1, t1, t2
	add	t1, t1, 0x40
	add	t0, t0, t1
	li	t1, 0x12
	sw	t1, 0(t0)

poll_l2_idle:
	# Polling L2 idle status for core0
	lw	t1, 0x80(t4)
	andi	t1, t1, 0xf
	bnez	t1, poll_l2_idle

	# disable L2
	lw	t1, 0x8(t4)
	srli	t1, t1, 1
	slli	t1, t1, 1
	sw	t1, 0x8(t4)

goto_sleep:
store_sp:
	# store sp to pcs scratch for each core
	li	t0, 0x20
	li	t1, 3
	csrr	t2, CSR_MHARTID
	add	t1, t1, t2
	mul	t0, t0, t1
	addi	t0, t0,	0x84
	la	t1, smu
	REG_L	t1, 0(t1)
	add	t0, t0,	t1
	sw	sp, 0(t0)

	wfi

sleep_hang:
	j sleep_hang

	.section .text, "ax", %progbits
	.align 3
	.global cpu_resume
cpu_resume:

	# Check if it comes from NMI, go resume if not.
	csrr	t0, CSR_MCAUSE
	beqz	t0, go_resume
	j	sleep_hang

go_resume:
enable_CM:
	# enable CM (DC_COHEN)
	csrr  t1, CSR_MCACHE_CTL
	lui   t2, 0x80
	or    t1, t1, t2
	csrw  CSR_MCACHE_CTL, t1

	# check DC_COHEN is enabled (25-series does not have CM)
	csrr  t1, CSR_MCACHE_CTL
	lui   t2, 0x80
	bne   t1, t2, enable_I_D_cache

wait_for_DC_COHSTA_is_enabled:
	csrr  t1, CSR_MCACHE_CTL
	srli  t1, t1, 12
	li    a5, 0x100
	and   t1, t1, a5
	beqz  t1, wait_for_DC_COHSTA_is_enabled

enable_I_D_cache:
	# enable d-cache & i-cache
	csrrs	t0, CSR_MCACHE_CTL, 0x1
	csrrs	t0, CSR_MCACHE_CTL, 0x2

restore_sp:
	# load sp
	li	t0, 0x20
	li	t1, 3
	csrr	t2, CSR_MHARTID
	add	t1, t1, t2
	mul	t0, t0, t1
	addi	t0, t0, 0x84
	la t1, smu
	REG_L t1, 0(t1)
	add	t0, t0, t1
	lw	sp, 0(t0)

suspend_mode_check2:
	# load ae350_suspend_mode[n] to $a4
	# $a4 = *(&ae350_suspend_mode + (mhartid * sizeof(int)))
	csrr  t1, CSR_MHARTID
	slli  t1, t1, 0x2
	la	  t0, ae350_suspend_mode
	add	  t0, t0, t1
	lw	  a4, 0(t0)

	# check ae350_suspend_mode[n] == DeepSleepMode
	li t1, DeepSleepMode
	beq a4, t1, enable_L2

	# check ae350_suspend_mode[n] == CpuHotplugDeepSleepMode
	li t1, CpuHotplugDeepSleepMode
	beq a4, t1, restore_regs

	j sleep_hang

enable_L2:
	la t4, l2c
	REG_L t4, 0(t4)
	beq x0, t4, restore_regs

	csrr    t0, CSR_MHARTID
	bnez	t0, restore_regs

	# restore L2 setting
	POP(t1)
	sw	t1, 0x8(t4)

restore_regs:
	# resume cpu regisger
	# Pop pmp
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

	# Pop Andes s-mode reg
	POP_CSR(CSR_SCOUNTINHIBIT)
	POP_CSR(CSR_SCOUNTEROVF)
	POP_CSR(CSR_SCOUNTERMASK_U)
	POP_CSR(CSR_SCOUNTERMASK_S)
	POP_CSR(CSR_SCOUNTERMASK_M)
	POP_CSR(CSR_SCOUNTERINTEN)
	POP_CSR(CSR_SDCAUSE)
	POP_CSR(CSR_SLIP)
	POP_CSR(CSR_SLIE)

	# Pop RISC-V s-mode reg
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

	# check if PPMA exists
	csrr	t0, CSR_MMSC_CFG
	li	t1, 0x40000000	# 30th bit: PPMA
	and	t0, t0, t1
	beqz	t0, 2f
	# Pop Andes m-mode reg
	POP_CSR(PMAADDR_15)
	POP_CSR(PMAADDR_14)
	POP_CSR(PMAADDR_13)
	POP_CSR(PMAADDR_12)
	POP_CSR(PMAADDR_11)
	POP_CSR(PMAADDR_10)
	POP_CSR(PMAADDR_9)
	POP_CSR(PMAADDR_8)
	POP_CSR(PMAADDR_7)
	POP_CSR(PMAADDR_6)
	POP_CSR(PMAADDR_5)
	POP_CSR(PMAADDR_4)
	POP_CSR(PMAADDR_3)
	POP_CSR(PMAADDR_2)
	POP_CSR(PMAADDR_1)
	POP_CSR(PMAADDR_0)
#if __riscv_xlen == 64
	POP_CSR(PMACFG_2)
	POP_CSR(PMACFG_0)
#else
	POP_CSR(PMACFG_3)
	POP_CSR(PMACFG_2)
	POP_CSR(PMACFG_1)
	POP_CSR(PMACFG_0)
#endif
	# Pop cache setting
2:	POP_CSR(CSR_MCACHE_CTL)

	POP_CSR(CSR_MCOUNTEROVF)
	POP_CSR(CSR_MCOUNTERMASK_U)
	POP_CSR(CSR_MCOUNTERMASK_S)
	POP_CSR(CSR_MCOUNTERMASK_M)
	POP_CSR(CSR_MCOUNTERINTEN)
	POP_CSR(CSR_MCOUNTERWEN)
	POP_CSR(CSR_MMISC_CTL)
	POP_CSR(CSR_MPFT_CTL)
	POP_CSR(CSR_MSLIDELEG)
	POP_CSR(CSR_MDCAUSE)
	POP_CSR(CSR_MXSTATUS)
	POP_CSR(CSR_MSP_BASE)
	POP_CSR(CSR_MSP_BOUND)
	POP_CSR(CSR_MHSP_CTL)

	# Pop RISC-V m-mode reg
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
	POP_CSR(CSR_MISA)
	POP_CSR(CSR_MSTATUS)

	# Pop x1~x31
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
