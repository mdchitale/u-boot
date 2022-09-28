// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Ventana Micro Systems Inc.
 *
 * Derived work from spl_sata.c
 */

#include <common.h>
#include <spl.h>
#include <errno.h>
#include <fat.h>
#include <nvme.h>
#include <init.h>

static int spl_nvme_load_image(struct spl_image_info *spl_image,
			       struct spl_boot_device *bootdev)
{
	int ret;
	struct blk_desc *blk_desc;

	ret = pci_init();
	if (ret < 0)
		goto out;

	ret = nvme_scan_namespace();
	if (ret < 0)
		goto out;

	blk_show_device(IF_TYPE_NVME, CONFIG_SPL_NVME_BOOT_DEVICE);
	blk_desc = blk_get_devnum_by_type(IF_TYPE_NVME,
					  CONFIG_SPL_NVME_BOOT_DEVICE);
	if (IS_ENABLED(CONFIG_SPL_FS_FAT))
		ret = spl_load_image_fat(spl_image, bootdev, blk_desc,
					 CONFIG_SYS_NVME_FAT_BOOT_PARTITION,
					 CONFIG_SPL_PAYLOAD);
	else
		ret = -ENOSYS;

out:
	return ret;
}

SPL_LOAD_IMAGE_METHOD("NVME", 0, BOOT_DEVICE_NVME, spl_nvme_load_image);
