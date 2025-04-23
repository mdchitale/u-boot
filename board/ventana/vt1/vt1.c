// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024, Ventana Micro Systems Inc.
 */

#include <dm.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/ofnode.h>
#include <env.h>
#include <fdtdec.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <init.h>
#include <virtio_types.h>
#include <virtio.h>
#define RV_ETRACE_PARAM_MAX_LEN	64
DECLARE_GLOBAL_DATA_PTR;

int rv_etrace_parse_params(const char *params[RV_ETRACE_PARAM_MAX_LEN],
		     bool show_params);
static const char *itrace_params_qemu[RV_ETRACE_PARAM_MAX_LEN] = {
	".packet.srcid_bytes_p,0",
	".packet.tstamp_bytes_p,0",
	".packet.type_width_p,0",
	".itrace.arch_p,0",
	".itrace.blocks_p,0",
	".itrace.bpred_size_p,5",
	".itrace.cache_size_p,0",
	".itrace.call_counter_size_p,1",
	".itrace.ctype_width_p,0",
	".itrace.context_width_p,0",
	".itrace.time_width_p,0",
	".itrace.ecause_width_p,6",
	".itrace.f0s_width_p,1",
	".itrace.filter_context_p,0",
	".itrace.filter_time_p,0",
	".itrace.filter_excint_p,0",
	".itrace.filter_privilege_p,0",
	".itrace.filter_tval_p,0",
	".itrace.iaddress_lsb_p,0",
	".itrace.iaddress_width_p,64",
	".itrace.iretire_width_p,0",
	".itrace.ilastsize_width_p,1",
	".itrace.itype_width_p,0",
	".itrace.nocontext_p,1",
	".itrace.notime_p,1",
	".itrace.privilege_width_p,3",
	".itrace.retires_p,1",
	".itrace.return_stack_size_p,1",
	".itrace.sijump_p,0",
	".itrace.impdef_width_p,0",
	"",
};

#if IS_ENABLED(CONFIG_MTD_NOR_FLASH)
int is_flash_available(void)
{
	if (!ofnode_equal(ofnode_by_compatible(ofnode_null(), "cfi-flash"),
			  ofnode_null()))
		return 1;

	return 0;
}
#endif

int board_init(void)
{
	/*
	 * Make sure virtio bus is enumerated so that peripherals
	 * on the virtio bus can be discovered by their drivers
	 */
	virtio_init();

	return 0;
}

int board_late_init(void)
{
	ulong kernel_start;
	ofnode chosen_node;
	int ret;

	chosen_node = ofnode_path("/chosen");
	if (!ofnode_valid(chosen_node)) {
		debug("No chosen node found, can't get kernel start address\n");
		return 0;
	}

	if (CONFIG_IS_ENABLED(CMD_VMSTRACE)) {
		if (rv_etrace_parse_params(itrace_params_qemu, false)) {
			printf("Parsing e-trace params failed.\n");
		}
	}

#ifdef CONFIG_ARCH_RV64I
	ret = ofnode_read_u64(chosen_node, "riscv,kernel-start",
			      (u64 *)&kernel_start);
#else
	ret = ofnode_read_u32(chosen_node, "riscv,kernel-start",
			      (u32 *)&kernel_start);
#endif
	if (ret) {
		debug("Can't find kernel start address in device tree\n");
		return 0;
	}

	env_set_hex("kernel_start", kernel_start);

	return 0;
}

#ifdef CONFIG_SPL
void board_boot_order(u32 *spl_boot_list)
{
	ofnode config_node;
	const char *bootdev;

	/* If ventana-boot-device property is present */
	config_node = ofnode_path("/config");
	if (ofnode_valid(config_node)) {
		bootdev = ofnode_read_string(config_node,
					     "ventana-boot-device");
		if (bootdev) {
			if (strcmp(bootdev, "ram") == 0)
				spl_boot_list[0] = BOOT_DEVICE_RAM;
			else if (strcmp(bootdev, "nvme"))
				spl_boot_list[0] = BOOT_DEVICE_NVME;
			else
				spl_boot_list[0] = BOOT_DEVICE_NONE;

			spl_boot_list[1] = BOOT_DEVICE_NONE;
			return;
		}
	}
	/* The list below can be expanded in future. */
	spl_boot_list[0] = BOOT_DEVICE_USB;
	spl_boot_list[1] = BOOT_DEVICE_MMC1;
	spl_boot_list[2] = BOOT_DEVICE_NVME;
	spl_boot_list[3] = BOOT_DEVICE_NONE;
}

void spl_board_prepare_for_boot(void)
{
	struct udevice *dev;
	int rc;

	rc = uclass_find_device(UCLASS_IOMMU, 0, &dev);
	if (!rc && dev) {
		rc = device_remove(dev, DM_REMOVE_NORMAL);
		if (rc)
			printf("Cannot remove IOMMU device '%s' (err=%d)\n",
			       dev->name, rc);
	}
}
#endif

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* boot using first FIT config */
	return 0;
}
#endif
