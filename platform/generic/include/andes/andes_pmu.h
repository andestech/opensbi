/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) Copyright (c) 2023 Andes Technology Corporation
 */

#ifndef _RISCV_ANDES_PMU_H
#define _RISCV_ANDES_PMU_H

#include <sbi/sbi_hart.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_pmu.h>

#ifdef CONFIG_ANDES_PMU

/*
 * There are 3 Andes firmware events monitored by L2C performance counter now.
 * 0 : Total L2-cache access count
 * 1 : L2-cache access count
 * 2 : L2-cache miss count
 */
#define ANDES_CUSTOM_FW_EVENT_MAX	3

int andes_pmu_init(const struct fdt_match *match);
int andes_pmu_extensions_init(const struct fdt_match *match,
			      struct sbi_hart_features *hfeatures);

#else

static inline int andes_pmu_init(const struct fdt_match *match)
{
	return 0;
}
static inline int andes_pmu_extensions_init(const struct fdt_match *match,
			      struct sbi_hart_features *hfeatures)
{
	return 0;
}

#endif /* CONFIG_ANDES_PMU */

#endif /* _RISCV_ANDES_PMU_H */
