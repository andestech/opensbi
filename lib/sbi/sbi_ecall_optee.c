#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_version.h>
#include <sbi/riscv_asm.h>
#include <sbi_utils/irqchip/fdt_irqchip_plic.h>
#include <sbi_utils/tee/tee_ecall_opteed.h>

#define SBI_DOMAIN_TRUSTED_INDEX	1

#define PLIC_SOURCES			0x7F
#define PLIC_IE_WORDS			((PLIC_SOURCES + 31) / 32)

static u32 plic_secure_ie[2][PLIC_IE_WORDS];
static u32 plic_secure_threshold[2];
static u32 plic_nonsecure_ie[2][PLIC_IE_WORDS];
static u32 plic_nonsecure_threshold[2];

struct optee_vector_table_t {
	u32 yield_call_entry;
	u32 fast_call_entry;
	u32 cpu_on_entry;
	u32 cpu_off_entry;
	u32 cpu_resume_entry;
	u32 cpu_suspend_entry;
	u32 intr_entry;
	u32 system_off_entry;
	u32 system_reset_entry;
};

static struct optee_vector_table_t *optee_vect;

static void nonsecure_plic_save(void)
{
	fdt_plic_context_save(true,
			      (u32 *)&plic_nonsecure_ie[current_hartid()],
			      &plic_nonsecure_threshold[current_hartid()],
			      PLIC_IE_WORDS);

	fdt_plic_context_save(false,
			      (u32 *)&plic_secure_ie[current_hartid()],
			      &plic_secure_threshold[current_hartid()],
			      PLIC_IE_WORDS);
}

static void nonsecure_plic_restore(void)
{
	fdt_plic_context_restore(true,
				 (const u32 *)&plic_nonsecure_ie[current_hartid()],
				 plic_nonsecure_threshold[current_hartid()],
				 PLIC_IE_WORDS);
	fdt_plic_context_restore(false,
				 (const u32 *)&plic_secure_ie[current_hartid()],
				 plic_secure_threshold[current_hartid()],
				 PLIC_IE_WORDS);
}

static void secure_plic_save(void)
{
	fdt_plic_context_save(true,
			      (u32 *)&plic_secure_ie[current_hartid()],
			      &plic_secure_threshold[current_hartid()],
			      PLIC_IE_WORDS);

	fdt_plic_context_save(false,
			      (u32 *)&plic_nonsecure_ie[current_hartid()],
			      &plic_nonsecure_threshold[current_hartid()],
			      PLIC_IE_WORDS);
}

static void secure_plic_restore(void)
{
	fdt_plic_context_restore(true,
				 (const u32 *)&plic_secure_ie[current_hartid()],
				 plic_secure_threshold[current_hartid()],
				 PLIC_IE_WORDS);
	fdt_plic_context_restore(false,
				 (const u32 *)&plic_nonsecure_ie[current_hartid()],
				 plic_nonsecure_threshold[current_hartid()],
				 PLIC_IE_WORDS);
}


static int optee_call(u32 hartid, const struct sbi_trap_regs *regs, bool fast)
{
	int rc = 0;
	struct sbi_domain_hart_context *context = NULL;

	/* Save current domain context */
	rc = sbi_domain_save_hart_ctx(hartid, regs);
	if (rc) {
		sbi_printf("%s: Hart%d save context (error %d)\n",
			   __func__, hartid, rc);
		return rc;
	}

	/* Switch to target domain */
	rc = sbi_hart_switch_domain(hartid, SBI_DOMAIN_TRUSTED_INDEX);
	if (rc) {
		sbi_printf("%s: Switch hart%d to domain%d (error %d)\n",
			   __func__, hartid, SBI_DOMAIN_TRUSTED_INDEX, rc);
		return rc;
	}

	/* Get target domain context */
	context = sbi_domain_get_hart_context(hartid);
	if (!context) {
		sbi_printf("%s: Get hart%d context error\n", __func__, hartid);
		return SBI_EINVAL;
	}

	/* Update call parameters a0~a7 */
	context->regs.a0 = regs->a0;
	context->regs.a1 = regs->a1;
	context->regs.a2 = regs->a2;
	context->regs.a3 = regs->a3;
	context->regs.a4 = regs->a4;
	context->regs.a5 = regs->a5;
	context->regs.a6 = regs->a6;
	context->regs.a7 = regs->a7;

	/* Clear other registers */
	context->regs.s0 = 0;
	context->regs.s1 = 0;
	context->regs.s2 = 0;
	context->regs.s3 = 0;
	context->regs.s4 = 0;
	context->regs.s5 = 0;
	context->regs.s6 = 0;
	context->regs.s8 = 0;
	context->regs.s9 = 0;
	context->regs.s10 = 0;
	context->regs.s11 = 0;
	context->regs.t0 = 0;
	context->regs.t1 = 0;
	context->regs.t2 = 0;
	context->regs.t3 = 0;
	context->regs.t4 = 0;
	context->regs.t5 = 0;
	context->regs.t6 = 0;

	if (fast)
		context->regs.mepc = (ulong)&optee_vect->fast_call_entry;
	else
		context->regs.mepc = (ulong)&optee_vect->yield_call_entry;

	/* Enter target domain */
	rc = sbi_hart_enter_domain(hartid, SBI_DOMAIN_TRUSTED_INDEX);
	if (rc) {
		sbi_printf("%s: Hart%d enter domain%d (error %d)\n",
			   __func__, hartid, SBI_DOMAIN_TRUSTED_INDEX, rc);
		return rc;
	}

	/* Should not be here */
	return SBI_EINVAL;
}

static int optee_return_call(u32 hartid, const struct sbi_trap_regs *regs)
{
	int rc = 0;
	struct sbi_domain_hart_context *context = NULL;

	/* Save current domain context */
	rc = sbi_domain_save_hart_ctx(hartid, regs);
	if (rc) {
		sbi_printf("%s: Hart%d save context (error %d)\n",
			   __func__, hartid, rc);
		return rc;
	}

	/* Switch to target domain */
	rc = sbi_hart_switch_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
	if (rc) {
		sbi_printf("%s: Switch hart%d to domain%d (error %d)\n",
			   __func__, hartid, SBI_DOMAIN_ROOT_INDEX, rc);
		return rc;
	}

	/* Get target domain context */
	context = sbi_domain_get_hart_context(hartid);
	if (!context) {
		sbi_printf("%s: Get hart%d context error\n", __func__, hartid);
		return SBI_EINVAL;
	}

	/* Update call parameters a0~a3 and mepc */
	context->regs.a0 = regs->a1;
	context->regs.a1 = regs->a2;
	context->regs.a2 = regs->a3;
	context->regs.a3 = regs->a4;
	context->regs.mepc += 4;

	/* Enter target domain */
	rc = sbi_hart_enter_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
	if (rc) {
		sbi_printf("%s: Hart%d enter domain%d (error %d)\n",
			   __func__, hartid, SBI_DOMAIN_ROOT_INDEX, rc);
		return rc;
	}

	/* Should not be here */
	return SBI_EINVAL;
}

static int optee_interrupt_entry(u32 hartid, const struct sbi_trap_regs *regs)
{
	int rc = 0;
	struct sbi_domain_hart_context *context = NULL;

	/* Save current domain context */
	rc = sbi_domain_save_hart_ctx(hartid, regs);
	if (rc) {
		sbi_printf("%s: Hart%d save context (error %d)\n",
			   __func__, hartid, rc);
		return rc;
	}

	/* Switch to target domain */
	rc = sbi_hart_switch_domain(hartid, SBI_DOMAIN_TRUSTED_INDEX);
	if (rc) {
		sbi_printf("%s: Switch hart%d to domain%d (error %d)\n",
			   __func__, hartid, SBI_DOMAIN_TRUSTED_INDEX, rc);
		return rc;
	}

	/* Get target domain context */
	context = sbi_domain_get_hart_context(hartid);
	if (!context) {
		sbi_printf("%s: Get hart%d context error\n", __func__, hartid);
		return SBI_EINVAL;
	}

	/* Set mepc as OP-TEE interrupt entry */
	context->regs.mepc = (ulong)&optee_vect->intr_entry;

	/* Enter target domain */
	rc = sbi_hart_enter_domain(hartid, SBI_DOMAIN_TRUSTED_INDEX);
	if (rc) {
		sbi_printf("%s: Hart%d enter domain%d (error %d)\n",
			   __func__, hartid, SBI_DOMAIN_TRUSTED_INDEX, rc);
		return rc;
	}

	/* Should not be here */
	return SBI_EINVAL;
}

void sbi_domain_dispatch_interrupt(struct sbi_trap_regs *regs)
{
	uint32_t hartid = current_hartid();
	struct sbi_domain *dom = sbi_domain_thishart_ptr();

	if (dom->index == SBI_DOMAIN_ROOT_INDEX) {
		/* Secure interrupt asserted when running non-secure OS. */
		nonsecure_plic_save();
		secure_plic_restore();

		/* Enter OP-TEE to handle interrupt. */
		optee_interrupt_entry(hartid, regs);
	} else {
		/* Non-secure interrupt asserted when running secure OS. */
		/* Assert virtual foreign interrupt for secure OS. */
		fdt_plic_set_pending(15);
		/* Mask non-secure interrupts. */
		csr_clear(CSR_MIE, MIP_MEIP);
	}
}

static int sbi_ecall_optee_handler(unsigned long extid, unsigned long funcid,
				   struct sbi_trap_regs *regs,
				   struct sbi_ecall_return *out)
{
	int ret = 0;
	struct sbi_domain *dom = sbi_domain_thishart_ptr();
	u32 hartid = current_hartid();
        struct sbi_scratch *scratch = sbi_hartid_to_scratch(hartid);
	int optee_funcid = regs->a0;

	/* From Non-secure */
	if (dom->index == SBI_DOMAIN_ROOT_INDEX) {
		nonsecure_plic_save();
		secure_plic_restore();

		ulong func_type = GET_ECALL_TYPE(optee_funcid);
		if (func_type == ECALL_TYPE_FAST) {
			ret = optee_call(hartid, regs, true);
		} else if (func_type == ECALL_TYPE_YEILD) {
			ret = optee_call(hartid, regs, false);
		} else
			return SBI_EFAIL;
		return ret;
	}

	switch (optee_funcid) {
	case SBI_EXT_OPTEE_RETURN_INIT_DONE:
		dom->booted = true;
		optee_vect = (struct optee_vector_table_t *)regs->a1;

		ret = sbi_domain_save_hart_ctx(hartid, regs);
		if (ret) {
			sbi_printf("%s: Hart%d save context (error %d)\n",
				__func__, hartid, ret);
			break;
		}
		ret = sbi_hart_switch_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
		if (ret) {
			sbi_printf("%s: Hart%d switch domain%d (error %d)\n",
				__func__, hartid, SBI_DOMAIN_ROOT_INDEX, ret);
			break;
		}
		secure_plic_save();
		nonsecure_plic_restore();
		/* Enter target domain */
		ret = sbi_hart_enter_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
		if (ret) {
			sbi_printf("%s: Hart%d enter domain%d (error %d)\n",
				__func__, hartid, SBI_DOMAIN_ROOT_INDEX, ret);
		}
		break;
	case SBI_EXT_OPTEE_RETURN_ON_DONE:
		ret = sbi_domain_save_hart_ctx(hartid, regs);
		if (ret) {
			sbi_printf("%s: Hart%d save context (error %d)\n",
				__func__, hartid, ret);
			break;
		}
		secure_plic_save();
		nonsecure_plic_restore();
		ret = sbi_hsm_hart_stop(scratch, false);
		if (ret != 0)
			sbi_printf("sbi_hsm_hart_stop failed %d\n", ret);
		if (!sbi_hsm_hart_change_state(scratch,
					       SBI_HSM_STATE_STOP_PENDING,
					       SBI_HSM_STATE_STOPPED))
			return SBI_EFAIL;
		ret = sbi_hart_switch_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
		if (ret) {
			sbi_printf("%s: Hart%d switch domain%d (error %d)\n",
				__func__, hartid, SBI_DOMAIN_ROOT_INDEX, ret);
			break;
		}
		sbi_hsm_hart_wait(scratch, hartid);
		sbi_hsm_hart_start_finish(scratch, hartid);
                //sbi_hsm_prepare_next_jump(scratch, hartid);
	        //sbi_hart_switch_mode(hartid, scratch->next_arg1,
		//	             scratch->next_addr,
		//	             scratch->next_mode, FALSE);
		break;
	case SBI_EXT_OPTEE_RETURN_CALL_DONE:
		secure_plic_save();
		nonsecure_plic_restore();
		ret = optee_return_call(hartid, regs);
		break;
	case SBI_EXT_OPTEE_RETURN_INTR_DONE:
		secure_plic_save();
		nonsecure_plic_restore();
		ret = sbi_hart_switch_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
		if (ret) {
			sbi_printf("%s: Hart%d switch domain%d (error %d)\n",
				__func__, hartid, SBI_DOMAIN_ROOT_INDEX, ret);
			break;
		}
		/* Enter target domain */
		ret = sbi_hart_enter_domain(hartid, SBI_DOMAIN_ROOT_INDEX);
		if (ret) {
			sbi_printf("%s: Hart%d enter domain%d (error %d)\n",
				__func__, hartid, SBI_DOMAIN_ROOT_INDEX, ret);
		}
		break;
	default:
		sbi_printf("Not support OPTEE funcid = %X\n", optee_funcid);
		sbi_hart_hang();
		ret = SBI_ENOTSUPP;
	}

	if (ret) {
		sbi_printf("Something Wrong, OPTEE funcid = %X\n",
			   optee_funcid);
		sbi_hart_hang();
	}

	return ret;
}

struct sbi_ecall_extension ecall_optee;

static int sbi_ecall_optee_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_optee);
}

struct sbi_ecall_extension ecall_optee = {
	.extid_start		= SBI_EXT_OPTEE,
	.extid_end		= SBI_EXT_OPTEE,
	.register_extensions	= sbi_ecall_optee_register_extensions,
	.handle			= sbi_ecall_optee_handler,
};
