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

DECLARE_GLOBAL_DATA_PTR;

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
