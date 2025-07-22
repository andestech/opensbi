/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 Andes Technology Corporation
 */

#ifndef _REMOTEPROC_H_
#define _REMOTEPROC_H_

#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_ipi.h>

#define ANDES_REMOTEPROC	"Andes Remote Processor"
#define SP_SEND_IPI_TO_MP	0x5
#define MBOX_GET_MSG		-1
#define MBOX_NO_MSG		1
#define MBOX_OFF		offsetof(struct swmsg_box, sp_mbox)

extern u32	remoteproc_mp_hartid;
extern u32	remoteproc_sp_hartid;
extern u64	remoteproc_swmbox;
extern u8	remoteproc_sp_enable;

/* SW definition message box for sp and for opensbi */
struct swmsg_box {
	u32 sp_mbox;
	u32 opensbi_mbox;
	u32 mp_status;
	u32 sp_status;
};

#ifdef CONFIG_ANDES_REMOTEPROC
void remoteproc_notify_mp_ipi(u32 target_hart, u32 source);
void remoteproc_init(bool cold_boot);
void remoteproc_ipi_enable(unsigned int enable);
uintptr_t remoteproc_get_init_func(void);
#else
inline void remoteproc_notify_mp_ipi(u32 target_hart, u32 source) {}
inline void remoteproc_init(bool cold_boot) {}
inline void remoteproc_ipi_enable(unsigned int enable) {}
static inline uintptr_t remoteproc_get_init_func(void) { return 0; }
#endif

#endif /* _REMOTEPROC_H_ */
