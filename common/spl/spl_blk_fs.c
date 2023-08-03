// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023
 * Ventana Micro Systems Inc.
 *
 */

#include <common.h>
#include <spl.h>
#include <image.h>
#include <fs.h>
#include <part.h>

struct blk_dev {
	const char *ifname;
	int devnum;
	int partnum;
	char dev_part_str[8];
};

static ulong spl_fit_read(struct spl_load_info *load, ulong file_offset,
			  ulong size, void *buf)
{
	loff_t actlen;
	int ret;
	struct blk_dev *dev = (struct blk_dev *)load->priv;

	ret = fs_set_blk_dev(dev->ifname, dev->dev_part_str, FS_TYPE_ANY);
	if (ret) {
		printf("spl: unable to set blk_dev %s %s. Err - %d\n",
		       dev->ifname, dev->dev_part_str, ret);
		return ret;
	}

	ret = fs_read(load->filename, (ulong)buf, file_offset, size, &actlen);
	if (ret < 0) {
		printf("spl: error reading image %s. Err - %d\n",
		       load->filename, ret);
		return ret;
	}

	return actlen;
}

static int spl_blk_file_size(struct blk_dev *dev, const char *filename,
			     loff_t *filesize)
{
	int ret;

	snprintf(dev->dev_part_str, sizeof(dev->dev_part_str) - 1, "%x:%x",
		 dev->devnum, dev->partnum);
	debug("Loading file %s from %s %s\n", filename, dev->ifname,
	      dev->dev_part_str);
	ret = fs_set_blk_dev(dev->ifname, dev->dev_part_str, FS_TYPE_ANY);
	if (ret) {
		printf("spl: unable to set blk_dev %s %s. Err - %d\n",
		       dev->ifname, dev->dev_part_str, ret);
		return ret;
	}

	ret = fs_size(filename, filesize);
	if (ret)
		printf("spl: unable to get size, file: %s. Err - %d\n",
		       filename, ret);
	return ret;
}

int spl_blk_load_image(struct spl_image_info *spl_image,
		       struct spl_boot_device *bootdev,
		       enum uclass_id uclass_id, int devnum, int partnum)
{
	const char *filename = CONFIG_SPL_FS_LOAD_PAYLOAD_NAME;
	struct blk_desc *blk_desc;
	loff_t filesize;
	struct blk_dev dev;
	int ret, part;
	struct spl_load_info load = {
		.read = spl_fit_read,
		.bl_len = 1,
		.filename = filename,
		.priv = &dev,
	};

	blk_desc = blk_get_devnum_by_uclass_id(uclass_id, devnum);
	if (!blk_desc) {
		printf("blk desc for %d %d not found\n", uclass_id, devnum);
		return ret;
	}

	blk_show_device(uclass_id, devnum);
	dev.ifname = blk_get_uclass_name(uclass_id);
	dev.devnum = devnum;
	/*
	 * First try to boot from EFI System partition. In case of failure,
	 * fall back to the configured partition.
	 */
	if (IS_ENABLED(CONFIG_SPL_ESP_BOOT)) {
		part = part_get_esp(blk_desc);
		if (part) {
			dev.partnum = part;
			ret = spl_blk_file_size(&dev, filename, &filesize);
			if (!ret)
				goto out;
		}
	}

	dev.partnum = partnum;
	ret = spl_blk_file_size(&dev, filename, &filesize);
	if (ret)
		return ret;
out:
	return spl_load(spl_image, bootdev, &load, filesize, 0);
}
