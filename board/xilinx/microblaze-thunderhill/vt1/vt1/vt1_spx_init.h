/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#ifndef __VT1_SPX_INIT_H__
#define __VT1_SPX_INIT_H__

#include <vmsplat_types.h>
#include <vt1/hw/vt1_lwic_mmr.h>
#include <vt1/hw/vt1_smrc_mmr.h>
#include <vt1/hw/vt1_dbmd_mmr.h>
#include <vt1/vt1_pcl_init.h>

#define VT1_CKGN_EFUSEATADDR_OFFSET(__i) (VT1_CKGN_EFUSEATADDR0_OFFSET + __i)

/* In nano seconds - Minimum 8 REFCLK cycles plus 8 SCLK cycles */
#define VT1_MTIME_DIV_DELAY	150000

/* Possible values for mtimer_div for REFCLK -> 100MHz
 * Divider -> MTIME Increment Frequency
 * 0x0(div: 1) -> 100MHz
 * 0x1(div: 2) -> 50MHz
 * 0x2(div: 4) -> 25MHz
 * 0x3(div: 8) -> 12.5MHz
 **/
#define VT1_MTIMER_DIV_0	0x0
#define VT1_MTIMER_DIV_1	0x1
#define VT1_MTIMER_DIV_2	0x2
#define VT1_MTIMER_DIV_3	0x3

/* Both input and multiplier are positive integers */
#define spx_round_down(f, m)	((f/m)*m)
#define spx_round_up(f, m)	((f + m - 1)/m)*m
#define spx_round_closest(f, m)					\
({								\
	typeof(f) d = f - (f/m)*m;				\
	(d < (m >> 1))? spx_round_down(f, m) : spx_round_up(f, m);	\
})

/**
 * \defgroup VT1_SPX_INIT VT1 SPX Interface
 * @brief Functions implemented by VT1 PCL-SPX SW IP
 * for configuration and initialization of blocks in
 * VT1 PCL-SPX IP
 * @{
 */

/**
 * vt1_fuses: VT1 eFuse Settings
 */
struct vt1_fuses {
	vmsplat_uint16_t cpu_vector;
	vmsplat_uint8_t  num_ul3_slices;
	vmsplat_uint8_t  ul3_cap;
	vmsplat_uint16_t bclk_base_div;
	vmsplat_uint16_t bclk_max_div;
	vmsplat_uint16_t nclk_base_div;
	vmsplat_uint16_t nclk_max_div;
	vmsplat_uint16_t cclk_base_div;
	vmsplat_uint16_t cclk_max_div;
};

struct vt1_mmio_bases {
	/* SPX system-wide base addresses */
	vmsplat_uint64_t DBID_DVec_base;
	vmsplat_uint64_t DBID_DReg_base;
	vmsplat_uint64_t DBID_PuC_base;
	vmsplat_uint64_t LWIC_Mtime_base;
	vmsplat_uint64_t SMRC_base;
	vmsplat_uint64_t CHIB_base;
	vmsplat_uint64_t SDDI_base;
	vmsplat_uint64_t CKGN_base;
	vmsplat_uint64_t I3C_APB_base;
	vmsplat_uint64_t PCL_regbus_base;
	/* CPX system-wide base addresses */
	vmsplat_uint64_t CDDI_base;
};

/**
 * @brief Read Efuse
 *
 * This function reads the per cluster efuse and
 * fills a structure struct vt1_fuses with various
 * values programmed in one time programmable efuse.
 *
 * NOTE: To get the read values in struct vt1_fuses
 * instance use the helper function vt1_spx_get_efuse
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] ckgn_base		CKGN Regbus Base Address
 * @param[in] num_harts		Number of Harts/CPUs
 *
 * @return enum vmsplat_error
 */
int vt1_read_fuses(vmsplat_uint32_t apcluster_idx,
		   vmsplat_uint64_t ckgn_base,
		   vmsplat_uint32_t num_harts);

/**
 * @brief Helper function to get initialized
 * struct vt1_fuses instance with per
 * cluster efuse values
 *
 * NOTE: This function does not actually reads the Efuse
 * which is done via vt1_read_fuses.
 * It must be called after calling vt1_read_fuses.
 *
 * Various internal settings for feature selection,
 * Enabling/Disabling various blocks and limiting
 * the number of CPUs in PCL are programmed in efuse.
 * This function reads the efuse and returns these
 * configurations.
 *
 * @param[in] apcluster_idx	Cluster ID
 *
 * @return struct vt1_fuses
 */
struct vt1_fuses *vt1_spx_get_efuse(vmsplat_uint32_t apcluster_idx);

/**
 * @brief Configure VT1 SATT
 *
 * SATT is System Address Translation Table which performs
 * mapping between 52-Bit System Address space to 24-Bit
 * SPRB Requester Address Space. SATT table contains
 * various RegBus regions base addresses for the system
 * address translation.
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] smrc_base		SMRC RegBus Base Address
 * @param[in] vt1_mmio		Pointer to SATT configuration instance
 *
 * @return enum vmsplat_error
 */
int vt1_config_satt(vmsplat_uint32_t apcluster_idx,
		    vmsplat_uint64_t smrc_base,
		    const struct vt1_mmio_bases *vt1_mmio);

/**
 * @brief Enable MTIME
 *
 * MTIME is a RISC-V Arch defined real-time counter
 * which increments at constant frequency.This function
 * enables the MTIME increment and configures the rate of
 * increment.
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] lwic_base		LWIC RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int vt1_enable_mtime(vmsplat_uint32_t apcluster_idx,
		     vmsplat_uint64_t lwic_base);

/**
 * @brief Configure PCL capability and bring its Non-Core
 * out of reset for further initialization and configuration
 * 
 * NOTE: This function must be called first before performing
 * any configuration or initialization of CPUs in Cluster
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] smrc_base		SMRC RegBus Base Address
 * @param[in] base_hartid	Base Hartid(RISC-V ARCH CPU ID) for Cluster
 * @param[in] cpu_vector	CPUs enabled in efuse
 * @param[in] num_ul3_slices	Number of UL3 Slices from efuse (num_harts-1)
 * @param[in] ul3_cap		Cache capacity of UL3(Cache lines)
 *
 * @return enum vmsplat_error
 */
int vt1_system_deassert(vmsplat_uint32_t apcluster_idx,
			vmsplat_uint64_t smrc_base,
			vmsplat_uint32_t base_hartid,
			vmsplat_uint16_t cpu_vector,
			vmsplat_uint8_t num_ul3_slices,
			vmsplat_uint8_t ul3_cap);
/**
 * @brief Enable VT1 Debug Module and configure
 * harts for debug
 * 
 * NOTE: This function must be called after performing
 * the system/cluster initialization, likely after the
 * vt1_cluster_deassert but before vt1_cluster_start
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] dbmd_base		Debug Module RegBus Base Address
 * @param[in] num_harts		Number of Harts available
 *
 * @return enum vmsplat_error
 */
int vt1_dbmd_enable(vmsplat_uint32_t apcluster_idx,
		    vmsplat_uint64_t dbmd_base,
		    vmsplat_uint32_t num_harts);

#endif /* __VT1_SPX_INIT_H__ */
