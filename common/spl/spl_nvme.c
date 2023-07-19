// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023
 * Ventana Micro Systems Inc.
 *
 */

#include <common.h>
#include <spl.h>
#include <init.h>
#include <nvme.h>

int __weak spl_nvme_boot_devpart(int *dev, int *part)
{
	*dev = CONFIG_SPL_NVME_BOOT_DEVICE;
	*part = CONFIG_SYS_NVME_BOOT_PARTITION;

	return 0;
}

static int spl_nvme_load_image(struct spl_image_info *spl_image,
			       struct spl_boot_device *bootdev)
{
	int ret, dev = -1, part = -1;

	ret = pci_init();
	if (ret < 0)
		return ret;

	ret = nvme_scan_namespace();
	if (ret < 0)
		return ret;

	ret = spl_nvme_boot_devpart(&dev, &part);
	if (ret < 0)
		return ret;

	ret = spl_blk_load_image(spl_image, bootdev, UCLASS_NVME, dev, part);
	return ret;
}

SPL_LOAD_IMAGE_METHOD("NVME", 0, BOOT_DEVICE_NVME, spl_nvme_load_image);
