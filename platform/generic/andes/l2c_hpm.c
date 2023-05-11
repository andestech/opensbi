#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi_utils/cache/cache.h>
#include <andes/andesv5.h>
#include <andes/l2c_hpm.h>

int ae350_fw_event_validate_code(uint32_t event_idx_code)
{
	if (is_andes(25)) /* 25-series doesn't support L2C HPM */
		return SBI_EINVAL;
	if (!cache_hpm_idle()) {
		/*
		 * We only use HPM counter 0, let the match fail if the
		 * counter is being used.
		 */
		return SBI_EINVAL;
	}

	switch (event_idx_code) {
	case SBI_PMU_FW_L2C_ACCESSES:	   /* event_idx: 0xf0016 */
	case SBI_PMU_FW_L2C_ACCESS_MISSES: /* event_idx: 0xf0017 */
		return SBI_PMU_EVENT_TYPE_FW;
	default:
		return SBI_EINVAL;
	}
}

bool ae350_fw_counter_match_code(uint32_t counter_index,
				 uint32_t event_idx_code)
{
	/*
	 * Allow any fw counter (totally 16) to record
	 * custom fw events
	 */
	return true;
}

uint64_t ae350_fw_counter_read_value(uint32_t counter_index)
{
	uint64_t val = 0;
	cache_read_hpm_ctr(&val);
	return val;
}

int ae350_fw_counter_start(uint32_t counter_index, uint32_t event_idx_code,
			   uint64_t init_val, bool init_val_update)
{
	if (init_val_update)
		cache_write_hpm_ctr(init_val);
	return cache_start_hpm(event_idx_code);
}

int ae350_fw_counter_stop(uint32_t counter_index)
{
	return cache_stop_hpm();
}
