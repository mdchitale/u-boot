// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025, Mayuresh Chitale <mchitale@ventanmicro.com>
 */

#include <command.h>
#include <console.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/delay.h>

#define RV_ETRACE_TRTE_CTRL_OFF 0x0
#define   RV_ETRACE_TRTE_ACTIVE 0
#define   RV_ETRACE_TRTE_ENABLE 1
#define   RV_ETRACE_TRTE_INSTTRACING 2
#define   RV_ETRACE_TRTE_EMPTY 3
#define   RV_ETRACE_TRTE_INSTMODE 4

#define RV_ETRACE_TRRAM_CONTROL_OFF 0x0
#define   RV_ETRACE_TRRAM_ACTIVE 0
#define   RV_ETRACE_TRRAM_ENABLE 1
#define   RV_ETRACE_TRRAM_EMPTY 3
#define   RV_ETRACE_TRRAM_MODE 4
#define   RV_ETRACE_TRRAM_STOPONWRAP 8
#define RV_ETRACE_TRRAM_STARTLOW_OFF 0x10
#define RV_ETRACE_TRRAM_STARTHIGH_OFF 0x14
#define RV_ETRACE_TRRAM_LIMITLOW_OFF 0x18
#define RV_ETRACE_TRRAM_LIMITHIGH_OFF 0x1c
#define RV_ETRACE_TRRAM_WPLOW_OFF 0x20
#define RV_ETRACE_TRRAM_WPHIGH_OFF 0x24
#define RV_ETRACE_TRRAM_RPLOW_OFF 0x28
#define RV_ETRACE_TRRAM_RPHIGH_OFF 0x2c
#define RV_ETRACE_PARAM_MAX_LEN	64

void rv_etrace_pktdump(void *buf, size_t len, size_t wp);
static void *v2_enc_base_g = NULL, *v2_ram_sink_base_g = NULL;
static u64 cpu_g = 0;

#define assert_reg_addr() \
	do { \
		if (unlikely(!v2_enc_base_g)) { \
			printf("Please run vmstrace setaddr first.\n"); \
			return 0; \
		} \
	} while(0)

static int vmstrace_get_arg(const char *str, u64 *val, int base)
{
	char *endptr;

	*val = simple_strtoull(str, &endptr, base);

	if (*endptr) {
		printf("Invalid arg %s\n", str);
		return -EINVAL;
	}

	return 0;
}

static int do_vmstrace_setaddr(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	int ret;
	u64 val;

	ret = vmstrace_get_arg(argv[1], &cpu_g, 10);
	if (ret < 0)
		return 0;

	ret = vmstrace_get_arg(argv[2], &val, 16);
	if (ret < 0)
		return 0;

	v2_enc_base_g = (void *)(val + cpu_g * 0x1000);
	v2_ram_sink_base_g = v2_enc_base_g + 0x100;
	return 0;
}

static inline int vmstrace_wait_bit(void *addr, int bit, int bitval, int timeout)
{
	int i = 10;
	u32 val;

	while (i--) {
		val = readl(addr);
		if (((val >> bit) & 0x1) == bitval)
			break;
		else
			udelay(timeout/10);
	}

	if (i < 0)
		printf("Setting bit %d to %d timed out. Reg %x\n", bit,
			bitval, val);

	return (i < 0);
}

static int do_vmstrace_setup(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	u32 start_lo, start_hi, lim_lo, lim_hi;
	u32 trte_ctrl, trtram_ctrl;
	u64 start, limit, flags;
	int ret;

	if (argc == 3) {
		printf("Error. Start or limit not specified");
		return 0;
	}

	assert_reg_addr();
	ret = vmstrace_get_arg(argv[1], &start, 16);
	if (ret < 0)
		return 0;

	ret = vmstrace_get_arg(argv[2], &limit, 16);
	if (ret < 0)
		return 0;

	ret = vmstrace_get_arg(argv[3], &flags, 16);
	if (ret < 0)
		return 0;

	if (limit <= start) {
		printf("Error. Limit address should be greater than start address\n");
		return 0;
	}

	start_lo = lower_32_bits(start);
	start_hi = upper_32_bits(start);
	lim_lo = lower_32_bits(limit);
	lim_hi = upper_32_bits(limit);

	/* Release components from reset */
	writel(0, v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);
	ret = vmstrace_wait_bit((void *)(v2_enc_base_g +
				RV_ETRACE_TRTE_CTRL_OFF),
				RV_ETRACE_TRTE_ACTIVE, 0, 1000);
	if (ret)
		return 0;

	writel(0x1, v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);
	ret = vmstrace_wait_bit((void *)(v2_enc_base_g +
				RV_ETRACE_TRTE_CTRL_OFF),
				RV_ETRACE_TRTE_ACTIVE, 1, 1000);
	if (ret)
		return 0;

	writel(0, v2_ram_sink_base_g + RV_ETRACE_TRRAM_CONTROL_OFF);
	ret = vmstrace_wait_bit((void *)(v2_ram_sink_base_g +
				RV_ETRACE_TRRAM_CONTROL_OFF),
				RV_ETRACE_TRRAM_ACTIVE, 0, 1000);
	if (ret)
		return 0;

	writel(0x1, v2_ram_sink_base_g + RV_ETRACE_TRRAM_CONTROL_OFF);
	ret = vmstrace_wait_bit((void *)(v2_ram_sink_base_g +
				RV_ETRACE_TRRAM_CONTROL_OFF),
				RV_ETRACE_TRRAM_ACTIVE, 1, 1000);
	if (ret)
		return 0;

	trte_ctrl = readl(v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);

	/* setup ram sink mode, addresses etc and enable it*/
	writel(start_lo, v2_ram_sink_base_g + RV_ETRACE_TRRAM_STARTLOW_OFF);
	writel(start_hi, v2_ram_sink_base_g + RV_ETRACE_TRRAM_STARTHIGH_OFF);
	writel(start_lo, v2_ram_sink_base_g + RV_ETRACE_TRRAM_WPLOW_OFF);
	writel(start_hi, v2_ram_sink_base_g + RV_ETRACE_TRRAM_WPHIGH_OFF);
	writel(lim_lo, v2_ram_sink_base_g + RV_ETRACE_TRRAM_LIMITLOW_OFF);
	writel(lim_hi, v2_ram_sink_base_g + RV_ETRACE_TRRAM_LIMITHIGH_OFF);

	trtram_ctrl = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_CONTROL_OFF);
	if (flags & 0x1)
		trtram_ctrl |= (1 << RV_ETRACE_TRRAM_STOPONWRAP);
	writel(trtram_ctrl, v2_ram_sink_base_g + RV_ETRACE_TRRAM_CONTROL_OFF);

	trtram_ctrl |= (1 << RV_ETRACE_TRRAM_ENABLE);
	writel(trtram_ctrl, v2_ram_sink_base_g + RV_ETRACE_TRRAM_CONTROL_OFF);
	ret = vmstrace_wait_bit((void *)(v2_ram_sink_base_g +
				RV_ETRACE_TRRAM_CONTROL_OFF),
				RV_ETRACE_TRRAM_ENABLE, 1, 10000);
	if (ret) {
		printf("Failed to set ttram_ctrl enable to\n");
		/* Fall through for now */
		//return 0;
	}
		
	trte_ctrl |= (0x6 << RV_ETRACE_TRTE_INSTMODE);
	writel(trte_ctrl, v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);

	trte_ctrl |= (1 << RV_ETRACE_TRTE_ENABLE);
	writel(trte_ctrl, v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);
	ret = vmstrace_wait_bit((void *)(v2_enc_base_g +
				RV_ETRACE_TRTE_CTRL_OFF),
				RV_ETRACE_TRTE_ENABLE, 1, 10000);
	if (ret) {
		printf("Failed to set trte_ctrl enable\n");
		return 0;
	}

	printf("E-Trace enabled. Ram sink: mode %s, start address %llx, "
		"limit address %llx\n",
		(trtram_ctrl & (1 << RV_ETRACE_TRRAM_MODE)) ? "SMEM" : "SRAM",
		start, limit);

	return 0;
}

static int do_vmstrace_start(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	u32 val;

	assert_reg_addr();
	val = readl(v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);
	val |= (1 << RV_ETRACE_TRTE_INSTTRACING);
	writel(val, v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);

	return 0;
}

static int do_vmstrace_stop(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	u32 val;

	assert_reg_addr();
	val = readl(v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);
	val &= ~(1 << RV_ETRACE_TRTE_INSTTRACING);
	writel(val, v2_enc_base_g + RV_ETRACE_TRTE_CTRL_OFF);

	return 0;
}

static int do_vmstrace_pktdump(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	u32 start_lo, start_hi, wp_lo, wp_hi, lim_lo, lim_hi;
	size_t start, limit, wp;

	assert_reg_addr();
	start_lo = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_STARTLOW_OFF);
	start_hi = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_STARTHIGH_OFF);
	start = (u64) start_hi << 32 | start_lo;

	lim_lo = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_LIMITLOW_OFF);
	lim_hi = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_LIMITHIGH_OFF);
	limit = (u64) lim_hi << 32 | lim_lo;

	wp_lo = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_WPLOW_OFF);
	wp_hi = readl(v2_ram_sink_base_g + RV_ETRACE_TRRAM_WPHIGH_OFF);
	wp = (u64)wp_hi << 32 | wp_lo;

	rv_etrace_pktdump((unsigned char *)start, limit - start, wp - start);

	return 0;
}

static struct cmd_tbl cmd_vmstrace[] = {
	U_BOOT_CMD_MKENT(setaddr, 4, 0, do_vmstrace_setaddr, "", ""),
	U_BOOT_CMD_MKENT(setup, 4, 0, do_vmstrace_setup, "", ""),
	U_BOOT_CMD_MKENT(start, 2, 0, do_vmstrace_start, "", ""),
	U_BOOT_CMD_MKENT(stop, 2, 0, do_vmstrace_stop, "", ""),
	U_BOOT_CMD_MKENT(pktdump, 2, 0, do_vmstrace_pktdump, "", ""),
};

static int do_vmstrace_ops(struct cmd_tbl *cmdtp, int flag, int argc,
		     char *const argv[])
{
	struct cmd_tbl *cp;

	cp = find_cmd_tbl(argv[1], cmd_vmstrace, ARRAY_SIZE(cmd_vmstrace));

	/* Drop the vmstrace command */
	argc--;
	argv++;

	if (cp == NULL || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp))
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	vmstrace, 9, 0, do_vmstrace_ops,
	"VMS E-Trace Sub System",
	"setaddr cpu trace_mmr_base - Choose CPU for subsequent E-trace operations\n"
	"vmstrace setup trace_ram_start trace_ram_limit flags - Prepare the chosen CPU for E-trace\n"
	"         using the provided start and limit RAM addresses and flags.\n" 
	"         supported flags:\n"
	"             1 - Stop on wrap\n"
	"             2 - Enable implicit return\n"
	"vmstrace start - Start E-trace for the chosen cpu\n"
	"vmstrace stop - Stop E-trace for the chosen cpu\n"
	"vmstrace pktdump - Dump the captured trace data for the chosen cpu\n"
	);
