/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2022, Ventana Micro Systems Inc.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DRAM_BASE

#define CONFIG_STANDALONE_LOAD_ADDR	(CONFIG_SYS_SDRAM_BASE + SZ_2M)

/* Dummy timer frequency used for early boot delays */
#define RISCV_SMODE_TIMER_FREQ		10000000

/* Environment options */

#ifndef CONFIG_SPL_BUILD
#define BOOT_TARGET_DEVICES(func) \
	func(VENTANA, ventana, na) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(SCSI, scsi, 0) \
	func(SCSI, scsi, 1) \
	func(NVME, nvme, 0) \
	func(NVME, nvme, 1) \
	func(VIRTIO, virtio, 0) \
	func(VIRTIO, virtio, 1) \
	func(DHCP, dhcp, na)

#include <config_distro_bootcmd.h>
#include <linux/stringify.h>

#define BOOTENV_DEV_VENTANA(devtypeu, devtypel, instance) \
	"bootcmd_ventana=" \
		"if env exists kernel_start; then " \
			"bootm ${kernel_start} - ${fdtcontroladdr};" \
		"fi;\0"

#define BOOTENV_DEV_NAME_VENTANA(devtypeu, devtypel, instance) \
	"ventana "

#define CFG_EXTRA_ENV_SETTINGS \
	"fdt_high=0xffffffffffffffff\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"kernel_addr_r=" __stringify(CONFIG_ENV_KERNEL_ADDR_R) "\0" \
	"fdt_addr_r=" __stringify(CONFIG_ENV_FDT_ADDR_R) "\0" \
	"scriptaddr=" __stringify(CONFIG_ENV_SCRIPTADDR) "\0" \
	"pxefile_addr_r=" __stringify(CONFIG_ENV_PXEFILE_ADDR_R) "\0" \
	"ramdisk_addr_r=" __stringify(CONFIG_ENV_RAMDISK_ADDR_R) "\0" \
	"verify=n\0" \
	BOOTENV
#endif

#endif /* __CONFIG_H */
