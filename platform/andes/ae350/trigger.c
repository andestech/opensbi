/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Andes Technology Corporation
 *
 * Authors:
 *   Zong Li <zong@andestech.com>
 */

#include <sbi/riscv_locks.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include "trigger.h"

/* Record the status of trigger used by M-mode */
static struct trigger_module trigger_modules[][TRIGGER_MAX] = {
  [0 ... AE350_HART_COUNT - 1][0 ... TRIGGER_MAX - 1] = {
    .used = 0, .type = TRIGGER_TYPE_MCONTROL} };
static int total_triggers = TRIGGER_MAX;
static spinlock_t trigger_lock = SPIN_LOCK_INITIALIZER;

void trigger_init(void)
{
	uintptr_t tselect, tinfo;
	int i;

	for (i = 0; i < TRIGGER_MAX; i++) {
		csr_write(CSR_TSELECT, i);
		tselect = csr_read(CSR_TSELECT);
		if (i != tselect)
			break;

		tinfo = csr_read(CSR_TINFO);
		if (tinfo == 1)
			break;
	}

	/* Only use the minimum number of trigger modules of all harts. */
	spin_lock(&trigger_lock);
	if (total_triggers > i)
		total_triggers = i;
	spin_unlock(&trigger_lock);
}

static int trigger_set_tselect(int val)
{
	uintptr_t ret;

	csr_write(CSR_TSELECT, val);
	ret = csr_read(CSR_TSELECT);

	if (ret != val)
		return -1;

	return 0;
}

static int trigger_set_tdata1(uintptr_t val)
{
	uintptr_t ret;

	csr_write(CSR_TDATA1, val);
	ret = csr_read(CSR_TDATA1);

	if (ret != val)
		return -1;

	return 0;
}

static int trigger_set_tdata2(uintptr_t val)
{
	uintptr_t ret;

	csr_write(CSR_TDATA2, val);
	ret = csr_read(CSR_TDATA2);

	if (ret != val)
		return -1;

	return 0;
}

static int trigger_set_tdata3(uintptr_t val)
{
	uintptr_t ret;

	csr_write(CSR_TDATA3, val);
	ret = csr_read(CSR_TDATA3);

	if (ret != val)
		return -1;

	return 0;
}

/* The triggers may be used by Debugger */
static int trigger_used_by_dmode(int num)
{
	int dmode;

	trigger_set_tselect(num);
	dmode = (csr_read(CSR_TDATA1) & TDATA1_OFFSET_DMOEE);

	return dmode;
}

static int trigger_get_free(void)
{
	int i, hartid = csr_read(CSR_MHARTID);

	for (i = 0; i < total_triggers; i++) {
		if (!trigger_modules[hartid][i].used
		    && !trigger_used_by_dmode(i))
			break;
	}

	return i;
}

static int trigger_get_used_by_type(int type)
{
	int i, hartid = csr_read(CSR_MHARTID);

	for (i = 0; i < total_triggers; i++) {
		if (trigger_modules[hartid][i].type == type
		    && trigger_modules[hartid][i].used)
			break;
	}

	return i;
}

/* If there is no used trigger of the type, find a free one */
static int trigger_get_available(int type)
{
	int num = trigger_get_used_by_type(type);

	if (!TRIGGER_SUPPORT(num)) {
		num = trigger_get_free();
	}

	return num;
}

int trigger_set_icount(uintptr_t count, uintptr_t  m, uintptr_t s,
		       uintptr_t u)
{
	uintptr_t val;
	int num, err;
	int hartid = csr_read(CSR_MHARTID);

	num = trigger_get_available(TRIGGER_TYPE_ICOUNT);

	if (!TRIGGER_SUPPORT(num)) {
		sbi_printf("machine mode: trigger %d is not supported.\n", num);
		return -1;
	}

	err = trigger_set_tselect(num);
	if (err)
		return -1;

	if (!TRIGGER_SUPPORT_TYPE(TRIGGER_TYPE_ICOUNT)) {
		sbi_printf("machine mode: trigger %d is not support %ld type.\n",
			   num, TRIGGER_TYPE_ICOUNT);
		return -1;
	}

	val = (TRIGGER_TYPE_ICOUNT << TDATA1_OFFSET_TYPE)
	       | (count << ICOUNT_OFFSET_COUNT)
	       | (m << ICOUNT_OFFSET_M)
	       | (s << ICOUNT_OFFSET_S)
	       | (u << ICOUNT_OFFSET_U);
	err = trigger_set_tdata1(val);
	if (err)
		return -1;

	val = (TEXTRA_SELECT_CONTEXT << TEXTRA_OFFSET_SSELECT)
	       | (1 << TEXTRA_OFFSET_SVALUE);
	err = trigger_set_tdata3(val);
	if (err)
		return -1;

	if (u) {
		trigger_modules[hartid][num].used = 1;
		trigger_modules[hartid][num].type = TRIGGER_TYPE_ICOUNT;
	} else {
		trigger_modules[hartid][num].used = 0;
	}

	return err;
}

int trigger_set_itrigger(uintptr_t interrupt, uintptr_t m, uintptr_t s,
			 uintptr_t u)
{
	uintptr_t val;
	int num, err;
	int hartid = csr_read(CSR_MHARTID);

	num = trigger_get_available(TRIGGER_TYPE_ITRIGGER);

	if (!TRIGGER_SUPPORT(num))
		return -1;
	if (!TRIGGER_SUPPORT(num)) {
		sbi_printf("machine mode: trigger %d is not supported.\n", num);
		return -1;
	}

	err = trigger_set_tselect(num);
	if (err)
		return -1;

	if (!TRIGGER_SUPPORT_TYPE(TRIGGER_TYPE_ITRIGGER)) {
		sbi_printf("machine mode: trigger %d is not support %ld type.\n",
			   num, TRIGGER_TYPE_ITRIGGER);
		return -1;
	}

	val = (TRIGGER_TYPE_ITRIGGER << TDATA1_OFFSET_TYPE)
	       | (m << ITRIGGER_OFFSET_M)
	       | (s << ITRIGGER_OFFSET_S)
	       | (u << ITRIGGER_OFFSET_U);
	err = trigger_set_tdata1(val);
	if (err)
		return -1;

	err = trigger_set_tdata2(interrupt);
	if (err)
		return -1;

	if (u) {
		trigger_modules[hartid][num].used = 1;
		trigger_modules[hartid][num].type = TRIGGER_TYPE_ITRIGGER;
	} else {
		trigger_modules[hartid][num].used = 0;
	}

	return err;
}

int trigger_set_etrigger(uintptr_t exception, uintptr_t m, uintptr_t s,
			 uintptr_t u)
{
	uintptr_t val;
	int num, err;
	int hartid = csr_read(CSR_MHARTID);

	num = trigger_get_available(TRIGGER_TYPE_ETRIGGER);

	if (!TRIGGER_SUPPORT(num)) {
		sbi_printf("machine mode: trigger %d is not supported.\n", num);
		return -1;
	}

	err = trigger_set_tselect(num);
	if (err)
		return -1;

	if (!TRIGGER_SUPPORT_TYPE(TRIGGER_TYPE_ETRIGGER)) {
		sbi_printf("machine mode: trigger %d is not support %ld type.\n",
			   num, TRIGGER_TYPE_ETRIGGER);
		return -1;
	}

	val = (TRIGGER_TYPE_ETRIGGER << TDATA1_OFFSET_TYPE)
	       | (m << ETRIGGER_OFFSET_M)
	       | (s << ETRIGGER_OFFSET_S)
	       | (u << ETRIGGER_OFFSET_U);
	err = trigger_set_tdata1(val);
	if (err)
		return -1;

	err = trigger_set_tdata2(exception);
	if (err)
		return -1;

	if (u) {
		trigger_modules[hartid][num].used = 1;
		trigger_modules[hartid][num].type = TRIGGER_TYPE_ETRIGGER;
	} else {
		trigger_modules[hartid][num].used = 0;
	}

	return err;
}
