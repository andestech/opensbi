#ifndef __ANDES_L2C_HPM_H__
#define __ANDES_L2C_HPM_H__

#include <sbi/sbi_types.h>

#define SBI_PMU_FW_L2C_ACCESSES SBI_PMU_FW_MAX
#define SBI_PMU_FW_L2C_ACCESS_MISSES (SBI_PMU_FW_MAX + 1)

int ae350_fw_event_validate_code(uint32_t event_idx_code);
bool ae350_fw_counter_match_code(uint32_t counter_index,
				 uint32_t event_idx_code);
uint64_t ae350_fw_counter_read_value(uint32_t counter_index);
int ae350_fw_counter_start(uint32_t counter_index, uint32_t event_idx_code,
			   uint64_t init_val, bool init_val_update);
int ae350_fw_counter_stop(uint32_t counter_index);

#endif /* __ANDES_L2C_HPM_H__ */
