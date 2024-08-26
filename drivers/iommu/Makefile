# SPDX-License-Identifier: GPL-2.0+

obj-$(CONFIG_$(SPL_)IOMMU) += iommu-uclass.o
obj-$(CONFIG_$(SPL_)RISCV_IOMMU) += iommu-riscv.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_APPLE_DART) += apple_dart.o
obj-$(CONFIG_SANDBOX) += sandbox_iommu.o
obj-$(CONFIG_QCOM_HYP_SMMU) += qcom-hyp-smmu.o
endif