// SPDX-License-Identifier: BSD-2-Clause
/*
 * fdt_fixup.h - Flat Device Tree manipulation helper routines
 * Implement platform specific DT fixups on top of libfdt. 
 *
 * Copyright (C) 2020 Bin Meng <bmeng.cn@gmail.com>
 */

#ifndef __FDT_FIXUP_H__
#define __FDT_FIXUP_H__

struct sbi_pmu_event_select_map {
	/**
	 * The description of an entry in
	 * riscv,event-to-mhpmevent property
	 */
	uint32_t eidx;
	uint64_t select;
};

struct sbi_pmu_event_counter_map {
	/**
	 * The description of an entry in
	 * riscv,event-to-mhpmcounters property
	 */
	uint32_t eidx_start;
	uint32_t eidx_end;
	uint32_t ctr_map;
};

struct sbi_pmu_raw_event_counter_map {
	/**
	 * The description of an entry in
	 * riscv,raw-event-to-mhpmcounters property
	 */
	uint64_t select;
	uint64_t select_mask;
	uint32_t ctr_map;
};

/**
 * Add PMU properties in the DT
 *
 * Add information about event to selector/counter mappings to the
 * devicetree.
 *
 * @param selects: array of event index to selector value mapping
 *                 descriptions, ending with empty element
 * @param counters: array of event indexes to counters mapping
 *                  descriptions, ending with empty element
 * @param rcounters: array of raw events to counters mapping
 *                   descriptions, ending with empty element
 * @return zero on success and -ve on failure
 */
int fdt_add_pmu(const struct sbi_pmu_event_select_map *selects,
		const struct sbi_pmu_event_counter_map *counters,
		const struct sbi_pmu_raw_event_counter_map *rcounters);

/**
 * Fix up the CPU node in the device tree
 *
 * This routine updates the "status" property of a CPU node in the device tree
 * to "disabled" if that hart is in disabled state in OpenSBI.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_cpu_fixup(void *fdt);

/**
 * Fix up the APLIC nodes in the device tree
 *
 * This routine disables APLIC nodes which are not accessible to the next
 * booting stage based on currently assigned domain.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_aplic_fixup(void *fdt);

/**
 * Fix up the IMSIC nodes in the device tree
 *
 * This routine disables IMSIC nodes which are not accessible to the next
 * booting stage based on currently assigned domain.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_imsic_fixup(void *fdt);

/**
 * Fix up the PLIC node in the device tree
 *
 * This routine updates the "interrupt-extended" property of the PLIC node in
 * the device tree to hide the M-mode external interrupt from CPUs.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_plic_fixup(void *fdt);

/**
 * Fix up the reserved memory node in the device tree
 *
 * This routine inserts a child node of the reserved memory node in the device
 * tree that describes the protected memory region done by OpenSBI via PMP.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 * @return zero on success and -ve on failure
 */
int fdt_reserved_memory_fixup(void *fdt);

/**
 * Fix up the reserved memory subnodes in the device tree
 *
 * This routine adds the no-map property to the reserved memory subnodes so
 * that the OS does not map those PMP protected memory regions.
 *
 * Platform codes must call this helper in their final_init() after fdt_fixups()
 * if the OS should not map the PMP protected reserved regions.
 *
 * @param fdt: device tree blob
 * @return zero on success and -ve on failure
 */
int fdt_reserved_memory_nomap_fixup(void *fdt);

/**
 * General device tree fix-up
 *
 * This routine do all required device tree fix-ups for a typical platform.
 * It fixes up the PLIC node, IMSIC nodes, APLIC nodes, and the reserved
 * memory node in the device tree by calling the corresponding helper
 * routines to accomplish the task.
 *
 * It is recommended that platform codes call this helper in their final_init()
 *
 * @param fdt: device tree blob
 */
void fdt_fixups(void *fdt);

#endif /* __FDT_FIXUP_H__ */

