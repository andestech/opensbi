/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Copyright (c) 2014, Linaro Limited. All rights reserved. */

#ifndef __TEEECALL_OPTEED_H__
#define __TEEECALL_OPTEED_H__

#define ECALL_TYPE_FAST     (1)
#define ECALL_TYPE_YEILD    (0)

#define FUNCID_TYPE_SHIFT   (31)
#define FUNCID_TYPE_MASK    (0x1)
#define ECALL_32            (0)
#define FUNCID_CC_SHIFT     (30)
#define FUNCID_OEN_SHIFT    (24)

#define FUNCID_NUM_MASK      (0xffff)

#define GET_ECALL_TYPE(id)        (((id) >> FUNCID_TYPE_SHIFT) & \
                     FUNCID_TYPE_MASK)


#define TEEECALL_OPTEED_RV(func_num) \
        ((ECALL_TYPE_FAST << FUNCID_TYPE_SHIFT) | \
         ((ECALL_32) << FUNCID_CC_SHIFT) | \
         (62 << FUNCID_OEN_SHIFT) | \
         ((func_num) & FUNCID_NUM_MASK))


/*
 * This file specify SMC function IDs used when returning from TEE to the
 * secure monitor.
 *
 * All SMC Function IDs indicates SMC32 Calling Convention but will carry
 * full 64 bit values in the argument registers if invoked from Aarch64
 * mode. This violates the SMC Calling Convention, but since this
 * convention only coveres API towards Normwal World it's something that
 * only concerns the OP-TEE Dispatcher in ARM Trusted Firmware and OP-TEE
 * OS at Secure EL1.
 */

/*
 * Issued when returning from initial entry.
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_ENTRY_DONE
 * r1/x1	Pointer to entry vector
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_ENTRY_DONE		0
#define TEEECALL_OPTEED_RETURN_ENTRY_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_ENTRY_DONE)



/*
 * Issued when returning from "cpu_on" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_ON_DONE
 * r1/x1	0 on success and anything else to indicate error condition
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_ON_DONE		1
#define TEEECALL_OPTEED_RETURN_ON_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_ON_DONE)

/*
 * Issued when returning from "cpu_off" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_OFF_DONE
 * r1/x1	0 on success and anything else to indicate error condition
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_OFF_DONE		2
#define TEEECALL_OPTEED_RETURN_OFF_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_OFF_DONE)

/*
 * Issued when returning from "cpu_suspend" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_SUSPEND_DONE
 * r1/x1	0 on success and anything else to indicate error condition
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_SUSPEND_DONE	3
#define TEEECALL_OPTEED_RETURN_SUSPEND_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_SUSPEND_DONE)

/*
 * Issued when returning from "cpu_resume" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_RESUME_DONE
 * r1/x1	0 on success and anything else to indicate error condition
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_RESUME_DONE		4
#define TEEECALL_OPTEED_RETURN_RESUME_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_RESUME_DONE)


/*
 * Issued when returning from "std_smc" or "fast_smc" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_CALL_DONE
 * r1-4/x1-4	Return value 0-3 which will passed to normal world in
 *		r0-3/x0-3
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_CALL_DONE		5
#define TEEECALL_OPTEED_RETURN_CALL_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_CALL_DONE)

/*
 * Issued when returning from "fiq" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_FIQ_DONE
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_FIQ_DONE		6
#define TEEECALL_OPTEED_RETURN_FIQ_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_FIQ_DONE)

/*
 * Issued when returning from "system_off" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_SYSTEM_OFF_DONE
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_SYSTEM_OFF_DONE	7
#define TEEECALL_OPTEED_RETURN_SYSTEM_OFF_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_SYSTEM_OFF_DONE)

/*
 * Issued when returning from "system_reset" vector
 *
 * Register usage:
 * r0/x0	SMC Function ID, TEEECALL_OPTEED_RETURN_SYSTEM_RESET_DONE
 */
#define TEEECALL_OPTEED_FUNCID_RETURN_SYSTEM_RESET_DONE	8
#define TEEECALL_OPTEED_RETURN_SYSTEM_RESET_DONE \
	TEEECALL_OPTEED_RV(TEEECALL_OPTEED_FUNCID_RETURN_SYSTEM_RESET_DONE)

#endif /* __TEEECALL_OPTEED_H__ */
