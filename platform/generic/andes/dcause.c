#include <andes/andesv5.h>

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>

static const char *const mdcause_fetch_access[] = {
	"Reserved",
	"ECC/Parity error",
	"PMP/Smepmp instruction access violation",
	"Bus error",
	"PMA empty hole access",
	"PMA attribute inconsistency",
	"PTW access device region",
	"Instruction cache multiple-hit"
};

static const char *const mdcause_illegal_instruction[] = {
	"Please parse mtval CSR",
	"FP diabled exception",
	"ACE diabled exception",
	"RVV disabled exception",
	"CCTL filllock/unlock not supported",
	"Reserved CCTL command",
	"CCTL TLB command not supported",
	"CCTL command privilege violation",
	"CCTL BTB command not supported",
	"CCTL L1D_VA_WB* and L1*_VA_INVAL not supported",
	"CCTL ALL type (L1D_*_ALL) not supported",
	"CCTL L1D_IX_WB* and L1*_IX_INVAL not supported",
	"Reserved XCSR"
};

static const char *const mdcause_load_access[] = {
	"Reserved",
	"ECC/Parity error",
	"PMP/Smepmp load access violation",
	"Bus error",
	"Misaligned address",
	"PMA empty hole access",
	"PMA attribute inconsistency",
	"PMA NAMO exception",
	"PTW access device region",
	"Data cache multiple-hit"
};

static const char *const mdcause_store_access[] = {
	"Reserved",
	"ECC/Parity error",
	"PMP/Smepmp store access violation",
	"Bus error",
	"Misaligned address",
	"PMA empty hole access",
	"PMA attribute inconsistency",
	"PMA NAMO exception",
	"PTW access device region",
	"Data cache multiple-hit",
	"CBO.Zero hits device region"
};

static const char *const mdcause_fetch_page_fault[] = {
	"Reserved",
	"ITLB multiple-hit"
};

static const char *const mdcause_load_page_fault[] = {
	"Reserved",
	"DTLB multiple-hit"
};

static const char *const mdcause_store_page_fault[] = {
	"Reserved",
	"Smepmp violation",
	"DTLB multiple-hit",
	"ITLB multiple-hit"
};

static const char *const mdcause_imprecise_ECC[] = {
	"Reserved",
	"LM slave port ECC/Parity error",
	"Imprecise store ECC/Parity error",
	"Imprecise load ECC/Parity error"
};

static const char *const mdcause_bus_rw_transaction[] = {
	"Reserved",
	"Bus read error",
	"Bus write error",
	"Read PMP/Smepmp check error",
	"Write PMP/Smepmp check error",
	"Read PMA check error",
	"Write PMA check error",
};

void print_detailed_cause(long mcause, ulong mdcause)
{
	if (mcause >= 0) {
		switch (mcause) {
		case CAUSE_FETCH_ACCESS:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_fetch_access[mdcause]);
			break;
		case CAUSE_ILLEGAL_INSTRUCTION:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_illegal_instruction[mdcause]);
			break;
		case CAUSE_LOAD_ACCESS:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_load_access[mdcause]);
			break;
		case CAUSE_STORE_ACCESS:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_store_access[mdcause]);
			break;
		case CAUSE_FETCH_PAGE_FAULT:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_fetch_page_fault[mdcause]);
			break;
		case CAUSE_LOAD_PAGE_FAULT:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_load_page_fault[mdcause]);
			break;
		case CAUSE_STORE_PAGE_FAULT:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_store_page_fault[mdcause]);
			break;
		default:
			sbi_printf("The detailed trap cause: %s\n",
				   "mdcause does not support this exception");
		}
	} else {
		mcause &= ~(1UL << (__riscv_xlen - 1));
		mdcause &= CSR_MDCAUSE_MASK;
		
		switch (mcause) {
		case CAUSE_IMPRECISE_ECC:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_imprecise_ECC[mdcause]);
			break;
		case CAUSE_BUS_RW_TRANSACTION:
			sbi_printf("The detailed trap cause: %s\n",
				   mdcause_bus_rw_transaction[mdcause]);
			break;
		case CAUSE_PMOVI:
			sbi_printf("The detailed trap cause: %s\n",
				   "Performance monitor overflow interrupt");
			break;
		default:
			sbi_printf("The detailed trap cause: %s\n",
				   "mdcause does not support this interrupt");
		}
	}
}
