#include <sbi/riscv_encoding.h>
#include "smu.h"
#include "platform.h"

.text
.global cpu_suspend2ram
.global cpu_resume

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
	PUSH_CSR(CSR_MEPC)
	PUSH_CSR(CSR_MCAUSE)
	PUSH_CSR(CSR_MTVAL)
	PUSH_CSR(CSR_MIP)
	PUSH_CSR(CSR_MCOUNTEREN)
	PUSH_CSR(CSR_MCOUNTINHIBIT)

	# Push Andes m-mode reg
	#PUSH_CSR(milmb)
	#PUSH_CSR(mdlmb)
	#PUSH_CSR(mecc_code)
	#PUSH_CSR(mnvec)
	PUSH_CSR(CSR_MCACHE_CTL)
	PUSH_CSR(CSR_MCCTLBEGINADDR)
	PUSH_CSR(CSR_MCCTLCOMMAND)
	PUSH_CSR(CSR_MCCTLDATA)
	#PUSH_CSR(mppib)
	#PUSH_CSR(mfiob)
	PUSH_CSR(CSR_MHSP_CTL)
	PUSH_CSR(CSR_MSP_BOUND)
	PUSH_CSR(CSR_MSP_BASE)
	PUSH_CSR(CSR_MXSTATUS)
	PUSH_CSR(CSR_MDCAUSE)
	PUSH_CSR(CSR_MSLIDELEG)
	#PUSH_CSR(msavestatus)
	#PUSH_CSR(msaveepc1)
	#PUSH_CSR(msavecause1)
	#PUSH_CSR(msaveepc2)
	#PUSH_CSR(msavecause2)
	#PUSH_CSR(msavedcause1)
	#PUSH_CSR(msavedcause2)
	PUSH_CSR(CSR_MPFT_CTL)
	PUSH_CSR(CSR_MMISC_CTL)
	PUSH_CSR(CSR_MCOUNTERWEN)
	PUSH_CSR(CSR_MCOUNTERINTEN)
	PUSH_CSR(CSR_MCOUNTERMASK_M)
	PUSH_CSR(CSR_MCOUNTERMASK_S)
	PUSH_CSR(CSR_MCOUNTERMASK_U)
	PUSH_CSR(CSR_MCOUNTEROVF)
	#PUSH_CSR(mirq_entry)
	#PUSH_CSR(mintsel_jal)
	#PUSH_CSR(pushmcause)
	#PUSH_CSR(pushmepc)
	#PUSH_CSR(pushmxstatus)
##if __riscv_xlen == 64
#	PUSH_CSR(pmacfg0)
#	PUSH_CSR(pmacfg2)
##else
#	PUSH_CSR(pmacfg0)
#	PUSH_CSR(pmacfg1)
#	PUSH_CSR(pmacfg2)
#	PUSH_CSR(pmacfg3)
##endif
#	PUSH_CSR(pmaaddr0)
#	PUSH_CSR(pmaaddr1)
#	PUSH_CSR(pmaaddr2)
#	PUSH_CSR(pmaaddr3)
#	PUSH_CSR(pmaaddr4)
#	PUSH_CSR(pmaaddr5)
#	PUSH_CSR(pmaaddr6)
#	PUSH_CSR(pmaaddr7)
#	PUSH_CSR(pmaaddr8)
#	PUSH_CSR(pmaaddr9)
#	PUSH_CSR(pmaaddr10)
#	PUSH_CSR(pmaaddr11)
#	PUSH_CSR(pmaaddr12)
#	PUSH_CSR(pmaaddr13)
#	PUSH_CSR(pmaaddr14)
#	PUSH_CSR(pmaaddr15)

	# Push RISC-V s-mode reg
	PUSH_CSR(CSR_SSTATUS)
	#PUSH_CSR(sedeleg)
	#PUSH_CSR(sideleg)
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
	PUSH_CSR(CSR_SCCTLDATA)
	#PUSH_CSR(smisc_ctl)

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

	# store sp to pcs scratch for each core
	li	t0, 0x20
	li	t1, 3
	csrr	t2, CSR_MHARTID
	add	t1, t1, t2
	mul	t0, t0, t1
	addi	t0, t0,	0x84
	li	t1, SMU_BASE
	add	t0, t0,	t1
	sw	sp, 0(t0)

	# store reset vector
	li	t0, 0x4
	csrr	t1, CSR_MHARTID
	mul	t0, t0, t1
	addi	t0, t0, 0x50
	li	t1, SMU_BASE
	add	t0, t0, t1
	la	t1, cpu_resume
	sw	t1, 0(t0)

	# flush dcache
	csrw	CSR_UCCTLCOMMAND, 0x6

	# disable d-cache
	csrrc	t0, CSR_MCACHE_CTL, 0x2

	# check if l2 exist
	li	t0, 0xe0500000
	lw	t1, 0(t0)
	li	t0, -1
	bne	t0, t1, no_l2

	# check if it is core 0
	csrr	t1, CSR_MHARTID
	bnez	t1, no_l2

	# flush and disable l2 by core 0
	li	t0, 0xe0500000
	li	t2, 0x10
	mul	t1, t1, t2
	add	t1, t1, 0x40
	add	t0, t0, t1
	li	t1, 0x12
	sw	t1, 0(t0)

poll_l2_idle:
	# Polling L2 idle status for core0
	li	t0, 0xe0500080
	lw	t1, 0(t0)
	andi	t1, t1, 0xf
	bnez	t1, poll_l2_idle

	# disable L2
	li	t0, 0xe0500008
	lw	t1, 0(t0)
	srli	t1, t1, 1
	slli	t1, t1, 1
	sw	t1, 0(t0)
no_l2:
	wfi

test_sec:
	j test_sec

.align 2
cpu_resume:

	# Check if it come from NMI, go resume if not.
	csrr	t0, CSR_MCAUSE
	beqz	t0, go_resume
cpu_hang:
	j	cpu_hang
go_resume:
	# load sp
	li	t0, 0x20
	li	t1, 3
	csrr	t2, CSR_MHARTID
	add	t1, t1, t2
	mul	t0, t0, t1
	addi	t0, t0, 0x84
	li	t1, SMU_BASE
	add	t0, t0, t1
	lw	sp, 0(t0)

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
	#POP_CSR(smisc_ctl)
	POP_CSR(CSR_SCCTLDATA)
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
	POP_CSR(CSR_SATP)
	POP_CSR(CSR_SIP)
	POP_CSR(CSR_STVAL)
	POP_CSR(CSR_SCAUSE)
	POP_CSR(CSR_SEPC)
	POP_CSR(CSR_SSCRATCH)
	POP_CSR(CSR_SCOUNTEREN)
	POP_CSR(CSR_STVEC)
	POP_CSR(CSR_SIE)
	#POP_CSR(sideleg)
	#POP_CSR(sedeleg)
	POP_CSR(CSR_SSTATUS)

	# Pop Andes m-mode reg
#	POP_CSR(pmaaddr15)
#	POP_CSR(pmaaddr14)
#	POP_CSR(pmaaddr13)
#	POP_CSR(pmaaddr12)
#	POP_CSR(pmaaddr11)
#	POP_CSR(pmaaddr10)
#	POP_CSR(pmaaddr9)
#	POP_CSR(pmaaddr8)
#	POP_CSR(pmaaddr7)
#	POP_CSR(pmaaddr6)
#	POP_CSR(pmaaddr5)
#	POP_CSR(pmaaddr4)
#	POP_CSR(pmaaddr3)
#	POP_CSR(pmaaddr2)
#	POP_CSR(pmaaddr1)
#	POP_CSR(pmaaddr0)
##if __riscv_xlen == 64
#	POP_CSR(pmacfg2)
#	POP_CSR(pmacfg0)
##else
#	POP_CSR(pmacfg3)
#	POP_CSR(pmacfg2)
#	POP_CSR(pmacfg1)
#	POP_CSR(pmacfg0)
##endif
	#POP_CSR(pushmxstatus)
	#POP_CSR(pushmepc)
	#POP_CSR(pushmcause)
	#POP_CSR(mintsel_jal)
	#POP_CSR(mirq_entry)
	POP_CSR(CSR_MCOUNTEROVF)
	POP_CSR(CSR_MCOUNTERMASK_U)
	POP_CSR(CSR_MCOUNTERMASK_S)
	POP_CSR(CSR_MCOUNTERMASK_M)
	POP_CSR(CSR_MCOUNTERINTEN)
	POP_CSR(CSR_MCOUNTERWEN)
	POP_CSR(CSR_MMISC_CTL)
	POP_CSR(CSR_MPFT_CTL)
	#POP_CSR(msavedcause2)
	#POP_CSR(msavedcause1)
	#POP_CSR(msavecause2)
	#POP_CSR(msaveepc2)
	#POP_CSR(msavecause1)
	#POP_CSR(msaveepc1)
	#POP_CSR(msavestatus)
	POP_CSR(CSR_MSLIDELEG)
	POP_CSR(CSR_MDCAUSE)
	POP_CSR(CSR_MXSTATUS)
	POP_CSR(CSR_MSP_BASE)
	POP_CSR(CSR_MSP_BOUND)
	POP_CSR(CSR_MHSP_CTL)
	#POP_CSR(mfiob)
	#POP_CSR(mppib)
	POP_CSR(CSR_MCCTLDATA)
	POP_CSR(CSR_MCCTLCOMMAND)
	POP_CSR(CSR_MCCTLBEGINADDR)
	POP_CSR(CSR_MCACHE_CTL)
	#POP_CSR(mnvec)
	#POP_CSR(mecc_code)
	#POP_CSR(mdlmb)
	#POP_CSR(milmb)

	# Pop RISC-V m-mode reg
	POP_CSR(CSR_MCOUNTINHIBIT)
	POP_CSR(CSR_MCOUNTEREN)
	POP_CSR(CSR_MIP)
	POP_CSR(CSR_MTVAL)
	POP_CSR(mcause)
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
	li	t0, 0xe0500000
	lw	t1, 0(t0)
	li	t0, -1
	beq	t0, t1, no_enable_l2

	csrr    t0, CSR_MHARTID
	bnez	t0, no_enable_l2

	# enable L2
	li	t0, 0xe0500008
	lw	t1, 0(t0)
	ori	t1, t1, 0x1
	sw	t1, 0(t0)

no_enable_l2:
	# enable d-cache
	csrrs	t0, CSR_MCACHE_CTL, 0x2
	ret
