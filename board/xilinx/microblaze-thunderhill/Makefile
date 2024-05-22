# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2024 Ventana Micro Systems
#
# Mayuresh Chitale <mchitale@ventanamicro.com>
# Derived from microblaze-generic BSP
#

obj-y = microblaze-thunderhill.o load_esp.o
obj-$(CONFIG_TARGET_THUNDERHILL_V1) +=vt1/
obj-$(CONFIG_TARGET_THUNDERHILL_V2) +=vt2/
ccflags-$(CONFIG_TARGET_THUNDERHILL_V1) += -I$(srctree)/board/xilinx/microblaze-thunderhill/vt1/
ccflags-$(CONFIG_TARGET_THUNDERHILL_V2) += -I$(srctree)/board/xilinx/microblaze-thunderhill/vt2/
