// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2024 Ventana Micro Systems
 *
 * Mayuresh Chitale<mchitale@ventanamicro.com>
 */

#include <config.h>
#include <env.h>
#include <init.h>
#include <log.h>
#include <serial.h>
#include <asm/global_data.h>
#include <dm/lists.h>
#include <fdtdec.h>
#include <linux/sizes.h>
#include <linux/stringify.h>
#include <linux/delay.h>
#define DEBUG
#ifdef CONFIG_TARGET_THUNDERHILL_V1
#include <vt1/cluster_init.h>
#elif CONFIG_TARGET_THUNDERHILL_V2
extern int cluster_init(int start);
#endif

extern int fat_boot_part_file_load(int tokc, char **tokv);
void spl_board_init_rest (void)
{

#ifdef CONFIG_TARGET_THUNDERHILL_V1
	char ch, *tokv[] = { "FIRMWARE", "ventana", "thunderhill-v1", "zstage.bin" };
#elif CONFIG_TARGET_THUNDERHILL_V2
	char ch, *tokv[] = { "FIRMWARE", "ventana", "thunderhill-v2", "zstage.bin" };
#endif
        int ret, i;

        printf("\nLoading %s/%s/%s/%s...", tokv[0], tokv[1], tokv[2], tokv[3]);
        ret = fat_boot_part_file_load(4, tokv);
        if (ret)
                printf("Failed\n");
        else
                printf("Done\n");

        for (i = 0; i < 100; i++)
                udelay(1000);

        cluster_init(1);

	ch = getchar();
	if (ch == 'r') {
		printf("Resetting...\n");
		for (i = 0; i < 100; i++)
			udelay(1000);
		writel(0x1, 0x10030000);
	}
	while(1);
}

#ifndef CONFIG_SPL_BUILD
int do_reset(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	__asm__ __volatile__ (
			"mts rmsr, r0;" \
			"brai " __stringify(CONFIG_XILINX_MICROBLAZE0_VECTOR_BASE_ADDR));

	return 0;
}

int dram_init(void)
{
	if (fdtdec_setup_mem_size_base() != 0)
		return -EINVAL;

	return 0;
};

#endif
