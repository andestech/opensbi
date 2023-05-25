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


    .section .text, "ax", %progbits
    .align 3
    .global cpu_suspend2ram
cpu_suspend2ram:
    # Store x1 ~ x31 to stack
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
    PUSH_CSR(CSR_MCACHE_CTL)

    # check if PPMA exists
    csrr    t0, CSR_MMSC_CFG
    li      t1, PPMA_EN
    and     t0, t0, t1
    beqz    t0, 1f
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

1:  # Push PMP
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

    # Push RISC-V s-mode reg
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

    # Push Andes s-mode reg
    PUSH_CSR(CSR_SLIE)
    PUSH_CSR(CSR_SLIP)
    PUSH_CSR(CSR_SDCAUSE)

    # Check if we need to store L2 base address and L2 settings
    # Get ae350_suspend_mode[n] to $a4 before CM is disabled
    # $a4 = *(&ae350_suspend_mode + (mhartid * sizeof(int)))
    csrr    t1, CSR_MHARTID
    slli    t1, t1, 0x2
    la      t0, ae350_suspend_mode
    add     t0, t0, t1
    lw      a4, 0(t0)

    # ae350_suspend_mode[n] == CpuHotplugDeepSleepMode --> jump to calc_rst_vec
    li      t1, CpuHotplugDeepSleepMode
    beq     a4, t1, calc_rst_vec

check_if_hart0:
    # Store L2 setting before disabling caches when doing deep sleep
    # Only store it on hart 0's stack
    csrr    t1, CSR_MHARTID
    bnez    t1, calc_rst_vec

alloc_L2_addr:
    # Store L2C base address & setting
    addi    sp, sp, -REGBYTES   # Allocate space from stack
    mv      a0, sp              # Use the allocated space as arg passed to cache_get_addr
    call    cache_get_addr
    REG_L   t0, 0(sp)           # Get addr from cache_get_addr to $t0
    addi    sp, sp, REGBYTES    # Free the allocated space
    bgez    a0, store_L2        # if $a0 < 0 then L2 does not exist and *addr is garbage
    li      a0, 0               # $a0 is used to store L2 addr (if $a0 == 0, L2 does not exist)
    j       calc_rst_vec        # Jump to calc_rst_vec
store_L2:
    mv      a0, t0              # $a0 is used to store L2 addr (if $a0 == 0, L2 does not exist)
    lw      t0, 8(t0)           # l2c base + 0x8: l2c control register offset
    PUSH(t0)                    # Store l2c setting

calc_rst_vec:
    li      t0, 0x4
    csrr    t1, CSR_MHARTID
    mul     t0, t0, t1
    addi    t0, t0, 0x50
eight_core:
    li      t1, 0x60
    blt     t0, t1, set_rst_vec
    addi    t0, t0, 0x1a0
set_rst_vec:
    la      t1, smu
    REG_L   t1, 0(t1)
    add     t0, t0, t1
    la      t1, cpu_resume
    sw      t1, 0(t0)

    #reset MIE
    csrw    CSR_MIE, 0
    csrw    CSR_MSTATUS, 0
    csrw    CSR_MIP, 0

    # reset SIE
    csrw    CSR_SIE, 0
    csrw    CSR_SSTATUS, 0
    csrw    CSR_SIP, 0

disable_I_D_cache:
    # flush dcache
    csrw    CSR_UCCTLCOMMAND, 0x6

    # disable d-cache & i-cache
    csrrc   t0, CSR_MCACHE_CTL, 0x2
    csrrc   t0, CSR_MCACHE_CTL, 0x1

disable_CM:
    # disable CM (DC_COHEN)
    csrr    t1, CSR_MCACHE_CTL
    lui     t2, 0xfff80
    addi    t2, t2, -1
    and     t1, t1, t2
    csrw    CSR_MCACHE_CTL, t1

wait_for_DC_COHSTA_disable:
    csrr    t1, CSR_MCACHE_CTL
    srli    t1, t1, 12
    li      a5, 0x100
    and     t1, t1, a5
    bnez    t1, wait_for_DC_COHSTA_disable

suspend_mode_check1:
    csrr    t1, CSR_MHARTID
    slli    t1, t1, 0x2
    la      t0, ae350_suspend_mode
    add     t0, t0, t1
    lw      a4, 0(t0)

    # ae350_suspend_mode[n] == CpuHotplugDeepSleepMode --> skip disable_L2
    li      t1, CpuHotplugDeepSleepMode
    beq     a4, t1, goto_sleep

disable_L2:
    # Check if l2 exist
    beqz    a0, goto_sleep

    # Check if it is core 0
    csrr    t1, CSR_MHARTID
    bnez    t1, goto_sleep

    # Write back invalidate L2
    mv      t0, a0
    li      t2, 0x10
    mul     t1, t1, t2
    add     t1, t1, 0x40
    add     t0, t0, t1
    li      t1, 0x12
    sw      t1, 0(t0)

poll_l2_idle:
    # Polling L2 idle status for core0
    lw      t1, 0x80(a0)
    andi    t1, t1, 0xf
    bnez    t1, poll_l2_idle

    # Disable L2
    lw      t1, 0x8(a0)
    srli    t1, t1, 1
    slli    t1, t1, 1
    sw      t1, 0x8(a0)

goto_sleep:
store_sp:
    # store sp to pcs scratch for each core
    li      t0, 0x20
    li      t1, 3
    csrr    t2, CSR_MHARTID
    add     t1, t1, t2
    mul     t0, t0, t1
    addi    t0, t0, 0x84
    la      t1, smu
    REG_L   t1, 0(t1)
    add     t0, t0, t1
    sw      sp, 0(t0)

    wfi

sleep_hang:
    j sleep_hang

    .section .text, "ax", %progbits
    .align 3
    .global cpu_resume
cpu_resume:

    # Check if it comes from NMI, go resume if not.
    csrr    t0, CSR_MCAUSE
    beqz    t0, go_resume
    j       sleep_hang

go_resume:
restore_sp:
    # load sp
    li      t0, 0x20
    li      t1, 3
    csrr    t2, CSR_MHARTID
    add     t1, t1, t2
    mul     t0, t0, t1
    addi    t0, t0, 0x84
    la      t1, smu
    REG_L   t1, 0(t1)
    add     t0, t0, t1
    lw      sp, 0(t0)

suspend_mode_check2:
    # load ae350_suspend_mode[n] to $a4
    # $a4 = *(&ae350_suspend_mode + (mhartid * sizeof(int)))
    csrr    t1, CSR_MHARTID
    slli    t1, t1, 0x2
    la      t0, ae350_suspend_mode
    add     t0, t0, t1
    lw      a4, 0(t0)

    # check ae350_suspend_mode[n] == CpuHotplugDeepSleepMode
    li      t1, CpuHotplugDeepSleepMode
    beq     a4, t1, enable_CM

enable_L2:
    csrr    t0, CSR_MHARTID
    bnez    t0, enable_CM	# if !(hart0), then jump to enable_CM
alloc_L2_addr_2:
    addi    sp, sp, -REGBYTES   # Allocate space from stack
    mv      a0, sp              # Use the allocated space as arg passed to cache_get_addr
    call    cache_get_addr
    REG_L   t0, 0(sp)           # Get addr from cache_get_addr to $t0
    addi    sp, sp, REGBYTES    # Free the allocated space
    bltz    a0, enable_CM
restore_L2:
    # restore L2 setting
    POP(t1)
    sw      t1, 0x8(t0)

enable_CM:
    # enable CM (DC_COHEN)
    csrr    t1, CSR_MCACHE_CTL
    lui     t2, 0x80
    or      t1, t1, t2
    csrw    CSR_MCACHE_CTL, t1

    # check DC_COHEN is enabled (25-series does not have CM)
    csrr    t1, CSR_MCACHE_CTL
    lui     t2, 0x80
    bne     t1, t2, enable_I_D_cache

wait_for_DC_COHSTA_is_enabled:
    csrr    t1, CSR_MCACHE_CTL
    srli    t1, t1, 12
    li      a5, 0x100
    and     t1, t1, a5
    beqz    t1, wait_for_DC_COHSTA_is_enabled

enable_I_D_cache:
    # enable d-cache & i-cache
    csrrs   t0, CSR_MCACHE_CTL, 0x1
    csrrs   t0, CSR_MCACHE_CTL, 0x2

restore_regs:
    # resume cpu regisger

    # Pop Andes s-mode reg
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

    # Pop PMP
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

    # check if PPMA exists
    csrr    t0, CSR_MMSC_CFG
    li      t1, PPMA_EN
    and     t0, t0, t1
    beqz    t0, 2f
    # Pop Andes m-mode reg
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

    # Pop cache setting
2:  POP_CSR(CSR_MCACHE_CTL)

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
