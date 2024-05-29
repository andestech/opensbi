/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems, Inc.
 *
 * Author(s):
 *   Himanshu Chauhan <hchauhan@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_ras.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/ras/fdt_ras.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/ras/riscv_reri_regs.h>
#include <sbi_utils/ras/ghes.h>

struct acpi_ghes_data {
	uint8_t init_done;
	uint64_t ghes_err_addr;
	uint64_t ghes_err_mem_sz;
	uint64_t ghes_err_addr_curr;
	uint64_t ghes_err_end_addr;
};

static struct acpi_ghes_data gdata;

static acpi_ghesv2 err_sources[MAX_ERR_SRCS];
static int err_src_alloc_idx;
uint64_t alloc_size[2];
uint64_t up_alloc_size[2];
static int ai = 0;

void dump_error_sources(void)
{
	int i = 0;
	static acpi_ghesv2 * ghes;

	sbi_printf("Allocted error sources: %d gdata init: %d\n", err_src_alloc_idx, gdata.init_done);
	sbi_printf("GHES Addr: 0x%lx\n", (ulong)gdata.ghes_err_addr);
	sbi_printf("GHES End Addr: 0x%lx (0x%lx)\n", (ulong)gdata.ghes_err_end_addr,
		   (ulong)(gdata.ghes_err_addr + gdata.ghes_err_mem_sz));
	sbi_printf("GHES Mem Size: 0x%lx\n", (ulong)gdata.ghes_err_mem_sz);
	for (i = 0; i < 2; i++) {
		sbi_printf("%d. Alloc Size: %lu Up Alloc: %lu\n",
			   i + 1, (ulong)alloc_size[i], (ulong)up_alloc_size[i]);
	}
	for (i = 0; i < err_src_alloc_idx; i++) {
		ghes = &err_sources[i];
		sbi_printf("Source ID: 0x%x Status Block: 0x%lx\n",
			   ghes->ghes.src_id, (ulong)ghes->ghes.gas.address);
	}
}

void acpi_ghes_init(uint64_t addr, uint64_t size)
{
	if (size == 0 || addr == 0)
		return;

	gdata.ghes_err_addr_curr = gdata.ghes_err_addr = addr;
	gdata.ghes_err_mem_sz = size;
	gdata.ghes_err_end_addr = (addr + size);
	gdata.init_done = 1;
}

#define ROUNDUP_2_64B(sz) ((size + 0x40) & ~(0x40 - 1))

static void *acpi_ghes_alloc(uint64_t size)
{
	uint64_t naddr;
	uint64_t nsz;

	if (!gdata.init_done)
		return NULL;

	/* if not multiple of 64-bytes */
	if (size & (0x40 - 1))
		/* round up to next 64 bytes */
		nsz = ROUNDUP_2_64B(size);
	else
		nsz = size;

	alloc_size[ai] = size;
	up_alloc_size[ai] = nsz;
	ai++;

	if (gdata.ghes_err_addr_curr + nsz >= gdata.ghes_err_end_addr) {
		return NULL;
	}

	naddr = gdata.ghes_err_addr_curr;
	gdata.ghes_err_addr_curr = gdata.ghes_err_addr_curr + nsz;

	return ((void *)(ulong)naddr);
}

int acpi_ghes_new_error_source(uint64_t err_src_id)
{
	acpi_ghesv2 *err_src;
	acpi_ghes_status_block *sblock;

	if (err_src_alloc_idx >= MAX_ERR_SRCS) {
		return SBI_EINVAL;
	}

	if (!gdata.init_done) {
		return SBI_EINVAL;
	}

	err_src = &err_sources[err_src_alloc_idx];
	err_src->ghes.type = ACPI_GHES_SOURCE_GENERIC_ERROR_V2;
	err_src->ghes.src_id = err_src_id;
	err_src->ghes.num_rec_pre_alloc = MAX_ERR_RECS;
	err_src->ghes.max_sec_per_rec = MAX_SECS_PER_REC;
	sblock = acpi_ghes_alloc(sizeof(*sblock));

	if (sblock == NULL) {
		sbi_printf("Not enough memory to allocate status block\n");
		return -1;
	}

	err_src->ghes.gas.address = (ulong)sblock;
	err_src_alloc_idx++;

	return 0;
}

static acpi_ghesv2 *find_error_source_by_id(uint8_t src_id)
{
	int i;
	acpi_ghesv2 *err_src;

	for (i = 0; i < err_src_alloc_idx; i++) {
		err_src = &err_sources[i];
		if (err_src->ghes.src_id == src_id)
			return err_src;
	}

	return NULL;
}

static uint64_t read_gas_u64(acpi_gas *gas)
{
	return *((uint64_t *)(ulong)gas->address);
}

static int ospm_acked_prev_err(acpi_gas *read_ack_register,
			       uint64_t ack_preserve, uint64_t ack_write)
{
	uint64_t resp;

	/* If there is no ack register, assume the previous error ack'ed */
	if (!read_ack_register->address)
		return 1;

	resp = read_gas_u64(read_ack_register);
	resp = *((volatile uint64_t *)(ulong)read_ack_register->address);
	resp &= ack_preserve;
	resp &= ack_write;

	return resp;
}

static void ghes_record_mem_error(acpi_ghes_status_block *error_block,
				  uint64_t error_physical_addr)
{
	/* Memory Error Section Type */
	const uuid_le uefi_cper_mem_sec =
		UUID_LE(0xA5BC1114, 0x6F64, 0x4EDE, 0xB8, 0x63, 0x3E, 0x83, \
			0xED, 0x7C, 0x83, 0xB1);
	uint32_t data_length;
	acpi_ghes_data_entry *dentry;
	cper_mem2_sec *msec;

	/* This is the length if adding a new generic error data entry*/
	data_length = ACPI_GHES_DATA_LENGTH + ACPI_GHES_MEM_CPER_LENGTH;

	/* Build the new generic error status block header */
	error_block->block_status = ACPI_GEBS_UNCORRECTABLE;
	error_block->raw_doffs = 0;
	error_block->raw_dlen = 0;
	error_block->data_len =  data_length;
	error_block->err_sev = ACPI_CPER_SEV_RECOVERABLE;

	/* Build generic data entry header */
	dentry = &error_block->entry;
	memcpy(dentry->type.type, &uefi_cper_mem_sec, sizeof(dentry->type));
	dentry->err_sev = ACPI_CPER_SEV_RECOVERABLE;
	dentry->vbits = 0;
	dentry->flags = 0;
	dentry->err_dlen = ACPI_GHES_MEM_CPER_LENGTH;
	memset(dentry->fru_id, 0, sizeof(dentry->fru_id));
	dentry->timestamp = 0;

	msec = &error_block->entry.cpers[0].sections[0].ms;
	memset(msec, 0, sizeof(*msec));
	msec->vbits |= 0x1UL;
	msec->phys_addr = error_physical_addr;
	msec->phys_addr_mask = (uint64_t)-1;
}

static void ghes_record_generic_cpu_error(acpi_ghes_status_block *error_block,
					  acpi_ghes_error_info *einfo)
{
	acpi_ghes_data_entry *dentry;
	cper_gen_proc_sec *psec;

	/* Generic CPU Error Section Type */
	const uuid_le uefi_cper_generic_cpu_sec =
		UUID_LE(0x9876CCAD, 0x47B4, 0x4bdb, 0xB6, 0x5E, 0x16,	\
			0xF1, 0x93, 0xC4, 0xF3, 0xDB);

	uint32_t data_length;

	/* This is the length if adding a new generic error data entry*/
	data_length = ACPI_GHES_DATA_LENGTH + ACPI_GHES_GENERIC_CPU_CPER_LENGTH;

	/* Build the generic error status block */
	error_block->block_status = ACPI_GEBS_UNCORRECTABLE;
	error_block->raw_doffs = 0;
	error_block->raw_dlen = 0;
	error_block->data_len =  data_length;
	error_block->err_sev = einfo->info.gpe.sev;

	/* Build generic data entry header */
	dentry = &error_block->entry;
	memcpy(dentry->type.type, &uefi_cper_generic_cpu_sec,
	       sizeof(dentry->type));
	dentry->err_sev = einfo->info.gpe.sev;
	dentry->vbits = 0;
	dentry->flags = 0;
	dentry->err_dlen = ACPI_GHES_GENERIC_CPU_CPER_LENGTH;
	memset(dentry->fru_id, 0, sizeof(dentry->fru_id));
	dentry->timestamp = 0;

	psec = &error_block->entry.cpers[0].sections[0].ps;
	psec->vbits = einfo->info.gpe.validation_bits;
	/* Processor Type */
	if (einfo->info.gpe.validation_bits & GPE_PROC_TYPE_VALID)
		psec->proc_type = einfo->info.gpe.proc_type;
	/* ISA */
	if (einfo->info.gpe.validation_bits & GPE_PROC_ISA_VALID)
		psec->proc_isa = einfo->info.gpe.proc_isa;
	/* Error Type */
	if (einfo->info.gpe.validation_bits & GPE_PROC_ERR_TYPE_VALID)
		psec->proc_err_type = einfo->info.gpe.proc_err_type;
	/* Operation */
	if (einfo->info.gpe.validation_bits & GPE_OP_VALID)
		psec->operation = einfo->info.gpe.operation;
	/* Flags */
	if (einfo->info.gpe.validation_bits & GPE_FLAGS_VALID)
		psec->flags = einfo->info.gpe.flags;
	/* Level */
	if (einfo->info.gpe.validation_bits & GPE_LEVEL_VALID)
		psec->level = einfo->info.gpe.level;

	/* Reserved field - must always be zero */
	psec->resvd = 0;

	/* CPU version */
	if (einfo->info.gpe.validation_bits & GPE_CPU_VERSION_VALID)
		psec->cpu_version_info = einfo->info.gpe.cpu_version;

	if (einfo->info.gpe.validation_bits & GPE_CPU_ID_VALID)
		psec->proc_id = einfo->info.gpe.cpu_id;

	if (einfo->info.gpe.validation_bits & GPE_TARGET_ADDR_VALID)
		psec->target_addr = einfo->info.gpe.target_addr;

	if (einfo->info.gpe.validation_bits & GPE_REQ_IDENT_VALID)
		psec->requestor_id = einfo->info.gpe.req_ident;

	if (einfo->info.gpe.validation_bits & GPE_RESP_IDENT_VALID)
		psec->responder_id = einfo->info.gpe.resp_ident;

	if (einfo->info.gpe.validation_bits & GPE_IP_VALID)
		psec->ins_ip = einfo->info.gpe.ip;
}

void acpi_ghes_record_errors(uint8_t source_id, acpi_ghes_error_info *einfo)
{
	acpi_ghesv2 *err_src;
	acpi_ghes_status_block *sblock;

	sbi_printf("%s: Error on source %d\n", __func__, source_id);
	err_src = find_error_source_by_id(source_id);
	if (!err_src) {
		sbi_printf("%s: Error Source %d not found\n", __func__,
			    source_id);
		return;
	}
	sbi_printf("%s: found error source of ID %d\n", __func__, source_id);
	if (!ospm_acked_prev_err(&err_src->ack_reg, err_src->ack_preserve,
				 err_src->ack_write)) {
		sbi_printf("OSPM hasn't acknoledged the previous error. New "
			   "error record cannot be created.\n");
		return;
	}

	/*
	 * FIXME: Read gas address via a function that respects the
	 * gas parameters. Don't read directly after typecast.
	 */
	sblock = (acpi_ghes_status_block *)(ulong)err_src->ghes.gas.address;

	if (einfo->etype == ERROR_TYPE_MEM && einfo->info.me.physical_address)
		ghes_record_mem_error(sblock, einfo->info.me.physical_address);
	else if (einfo->etype == ERROR_TYPE_GENERIC_CPU)
		ghes_record_generic_cpu_error(sblock, einfo);
}

int acpi_ghes_get_num_err_srcs(void)
{
	return err_src_alloc_idx;
}

int acpi_ghes_get_err_srcs_list(uint32_t *src_ids, uint32_t sz)
{
	int i;
	acpi_ghesv2 *src;

	if (!src_ids)
		return -SBI_EINVAL;

	if (sz > err_src_alloc_idx)
		return -SBI_EINVAL;

	src = &err_sources[0];

	for (i = 0; i < err_src_alloc_idx; i++) {
		src_ids[0] = src->ghes.src_id;
		src++;
	}

	return err_src_alloc_idx;
}

int acpi_ghes_get_err_src_desc(uint32_t src_id, acpi_ghesv2 *ghes)
{
	acpi_ghesv2 *g;

	g = find_error_source_by_id(src_id);

	if (g == NULL)
		return -SBI_ENOENT;

	memcpy(ghes, g, sizeof(acpi_ghesv2));

	return 0;
}
