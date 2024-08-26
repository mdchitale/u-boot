// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Ventana Micro Systems Ltd.
 */

#include <dm.h>
#include <iommu.h>
#include <asm/io.h>

#define RV_IOMMU_DDTP	0x10
#define RV_IOMMU_DDTP_MODE_MASK 0xf

enum RV_IOMMU_MODE {
	IOMMU_OFF,
	IOMMU_BARE,
};

struct rv_iommu_priv {
	void *base;
};

static const struct udevice_id rv_iommu_id[] = {
	{ .compatible = "riscv,iommu" },
	{ /* sentinel */ }
};

static void rv_iommu_set_mode(void *base, enum RV_IOMMU_MODE mode)
{
	unsigned int ddtp;

	ddtp = readl(base + RV_IOMMU_DDTP);
	ddtp &= ~RV_IOMMU_DDTP_MODE_MASK;
	ddtp |= mode;
	writel(ddtp, base + RV_IOMMU_DDTP);
}

static int rv_iommu_probe(struct udevice *dev)
{
	struct rv_iommu_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	rv_iommu_set_mode(priv->base, IOMMU_BARE);
	return 0;
}

static int rv_iommu_remove(struct udevice *dev)
{
	struct rv_iommu_priv *priv = dev_get_priv(dev);

	rv_iommu_set_mode(priv->base, IOMMU_OFF);
	return 0;
}

U_BOOT_DRIVER(rv_iommu) = {
	.name = "Risc-V-IOMMU",
	.id = UCLASS_IOMMU,
	.of_match = rv_iommu_id,
	.priv_auto = sizeof(struct rv_iommu_priv),
	.probe = rv_iommu_probe,
	.remove = rv_iommu_remove,
	.flags	= DM_FLAG_OS_PREPARE
};
