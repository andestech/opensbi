/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _DCAUSE_H_
#define _DCAUSE_H_

#ifdef CONFIG_PLATFORM_ANDES_AE350
void print_detailed_cause(long mcause, unsigned long mdcause);
#else
static void print_detailed_cause(long mcause, unsigned long mdcause) {}
#endif /* CONFIG_PLATFORM_ANDES_AE350 */

#endif /* _DCAUSE_H_ */
