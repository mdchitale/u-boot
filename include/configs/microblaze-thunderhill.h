/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2007-2010 Michal Simek
 *
 * Michal SIMEK <monstr@monstr.eu>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* Microblaze is microblaze_0 */
#define XILINX_FSL_NUMBER	3

/* uart */
/* The following table includes the supported baudrates */
# define CFG_SYS_BAUDRATE_TABLE \
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400}

/* SPL part */

#define CFG_SYS_UBOOT_BASE		CONFIG_TEXT_BASE

#endif	/* __CONFIG_H */
