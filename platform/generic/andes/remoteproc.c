#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_trap.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/ipi/andes_plicsw.h>
#include <andes/andes.h>
#include <andes/remoteproc.h>

u8		remoteproc_sp_enable;
u32		remoteproc_sp_hartid;
u32		remoteproc_mp_hartid;
u64		remoteproc_swmbox;
u64		rmem_base, rmem_size;
extern struct	plicsw_data plicsw;

void remoteproc_notify_mp_ipi(u32 target_hart, u32 source)
{
	if (target_hart == remoteproc_mp_hartid && source == SP_SEND_IPI_TO_MP) {
		if (!remoteproc_swmbox) {
			sbi_printf("%s get IPI event from SP core, but no swmbox\n",
				    ANDES_REMOTEPROC);
			return;
		}
		writel(1, (unsigned int *)(remoteproc_swmbox +
					   offsetof(struct swmsg_box, opensbi_mbox)));

		sbi_ipi_send_smode((1 << remoteproc_mp_hartid), 0x0);
	}
}

void remoteproc_ipi_enable(unsigned int enable)
{
	if (enable) {
		plicsw.hart_count += 1;
		remoteproc_sp_enable = 1;
	} else {
		plicsw.hart_count -= 1;
		remoteproc_sp_enable = 0;
	}
}

static int fdt_andes_swmbox(void *fdt)
{
	uint64_t reg_addr, reg_size;
	int parent;
	int subnode;
	const char *swmbox = "andes_swmbox";

	if (!fdt)
		goto warning;

	parent = fdt_path_offset(fdt, "/reserved-memory");
	if (parent < 0)
		return parent;

	fdt_for_each_subnode(subnode, fdt, parent) {
		if (subnode < 0)
			goto warning;

		fdt_get_node_addr_size(fdt, subnode, 0, &reg_addr, &reg_size);

		const char *name = fdt_get_name(fdt, subnode, NULL);

		if(strncmp(swmbox, name, sizeof(swmbox)) == 0) {
			sbi_printf("%s %s Addr:0x%lx Size:0x%lx\n", ANDES_REMOTEPROC,
				    swmbox, reg_addr, reg_size);
			remoteproc_swmbox = reg_addr;
			return 0;
		}
	}

warning:
	sbi_printf("reserved-memory parameter missing\n");
	return SBI_EINVAL;
}

static int fdt_andes_remoteproc(void *fdt)
{
	uint64_t reg_addr, reg_size;
	int parent;
	int len;
	u64 val64;
	const fdt32_t *val;

	if (!fdt)
		goto warning;

	parent = fdt_path_offset(fdt, "/andes_remoteproc_smp");
	if (parent < 0)
		goto warning;

	val = 0;
	val = fdt_getprop(fdt, parent, "mp_hartid", &len);
	if (val < 0)
		goto warning;

	remoteproc_mp_hartid = fdt32_to_cpu(*val);
	sbi_printf("%s mp hartid: %x\n", ANDES_REMOTEPROC, remoteproc_mp_hartid);

	val = 0;
	val = fdt_getprop(fdt, parent, "sp_hartid", &len);
	if (val < 0)
		goto warning;

	remoteproc_sp_hartid = fdt32_to_cpu(*val);
	sbi_printf("%s sp hartid: %x\n", ANDES_REMOTEPROC, remoteproc_sp_hartid);

	/* Read "rmem_base" DT property */
	val = fdt_getprop(fdt, parent, "rmem_base", &len);
	if (!val && len >= 8)
		return SBI_EINVAL;
	val64 = fdt32_to_cpu(val[0]);
	val64 = (val64 << 32) | fdt32_to_cpu(val[1]);
	reg_addr = val64;
	rmem_base = reg_addr;

	/* Read "rmem_size" DT property */
	val = fdt_getprop(fdt, parent, "rmem_size", &len);
	if (!val && len >= 8)
		return SBI_EINVAL;
	val64 = fdt32_to_cpu(val[0]);
	val64 = (val64 << 32) | fdt32_to_cpu(val[1]);
	reg_size = val64;
	rmem_size = reg_size;

	fdt_andes_swmbox(fdt);

	sbi_printf("%s Reserved Memory Addr:0x%lx Size:0x%lx\n",
		    ANDES_REMOTEPROC, reg_addr, reg_size);

	return 0;

warning:
	sbi_printf("andes_remoteproc_smp parameter missing\n");
	return SBI_EINVAL;
}

uintptr_t remoteproc_get_init_func(void)
{
	return (uintptr_t)&remoteproc_init;
}

void remoteproc_init(bool cold_boot)
{
	void *fdt;
	u32 hartid;
	int msg;

	csr_set(CSR_MCACHE_CTL, MCACHE_CTL_IC_EN |
				MCACHE_CTL_DC_EN |
				MCACHE_CTL_DC_COHEN_EN);
	hartid = current_hartid();
	if (cold_boot) {
		fdt = fdt_get_address();
		fdt_andes_remoteproc(fdt);
	} else {
		if (hartid == remoteproc_sp_hartid) {
			writel(MBOX_NO_MSG,
			       (unsigned int *)(remoteproc_swmbox + MBOX_OFF));

			while (1) {
				msg = readl((unsigned int *)(remoteproc_swmbox +
							     MBOX_OFF));
				if (msg == MBOX_GET_MSG)
					wfi();
			}
		}
	}
}
