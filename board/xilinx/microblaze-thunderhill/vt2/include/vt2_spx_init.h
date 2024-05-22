/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#ifndef __V2_SPX_INIT_H__
#define __V2_SPX_INIT_H__

#include <vmsplat_types.h>
#include <vt2_pcl_init.h>
#include <hw/vt2_lwic_mmr.h>
#include <hw/vt2_smrc_mmr.h>
#include <hw/vt2_dbmd_mmr.h>
#include <hw/vt2_chib_mmr.h>
#include <hw/vt2_ckgn_mmr.h>
#include <hw/vt2_chpl_mmr.h>

/* In nano seconds - Minimum 8 REFCLK cycles plus 8 SCLK cycles */
#define V2_MTIME_DIV_DELAY	150000

/* Possible values for mtimer_div for REFCLK -> 100MHz
 * Divider -> MTIME Increment Frequency
 * 0x0(div: 1) -> 100MHz
 * 0x1(div: 2) -> 50MHz
 * 0x2(div: 4) -> 25MHz
 * 0x3(div: 8) -> 12.5MHz
 **/
#define V2_MTIMER_DIV_0		0x0
#define V2_MTIMER_DIV_1		0x1
#define V2_MTIMER_DIV_2		0x2
#define V2_MTIMER_DIV_3		0x3

/* Both input and multiplier are positive integers */
#define spx_round_down(f, m)		((f/m)*m)
#define spx_round_up(f, m)		((f + m - 1)/m)*m
#define spx_round_closest(f, m)					\
({								\
	typeof(f) d = f - (f/m)*m;				\
	(d < (m >> 1))? spx_round_down(f, m) : spx_round_up(f, m);	\
})

/** V2 Regbus regions address space (offsets) */
#define V2_DBMD_DVEC_BASE		_ULL(0x0)
#define V2_DBMD_DREG_BASE		_ULL(0x1000)
#define V2_DBMD_PUC_BASE		_ULL(0x2000)
#define V2_LWIC_BASE			_ULL(0x4000)
#define V2_SMRC_BASE			_ULL(0x8000)
#define V2_CHIB_BASE			_ULL(0xc000)
#define V2_CHPL_BASE			_ULL(0x80000)
#define V2_CKGN_BASE			_ULL(0xc0000)
#define V2_I3C_IPAPB_BASE		_ULL(0xd0000)
#define V2_PCL_BASE			_ULL(0x1000000)

/** CHIB1 offset from the V2_CHIB_BASE */
#define V2_CHIB_CHIB1_OFFSET		_ULL(0x1000)

struct v2_fuses {
	vmsplat_uint8_t		cluster_id;
	vmsplat_uint16_t	cpu_vector;
	vmsplat_uint8_t		num_l3m_slices;
	vmsplat_uint8_t		l3m_cap;
	vmsplat_uint16_t	sclk_maxfbdiv;
	vmsplat_uint16_t	sclk_minrefdiv;
	vmsplat_uint16_t	nclk_maxfbdiv;
	vmsplat_uint16_t	nclk_minrefdiv;
	vmsplat_uint16_t	cclk_maxfbdiv;
	vmsplat_uint16_t	cclk_minrefdiv;
};

struct v2_mmio_bases {
	/* SPX system-wide base addresses */
	vmsplat_uint64_t	dbid_dvec_base;
	vmsplat_uint64_t	dbid_dreg_base;
	vmsplat_uint64_t	dbid_puc_base;
	vmsplat_uint64_t	lwic_mtime_base;
	vmsplat_uint64_t	smrc_base;
	vmsplat_uint64_t	chib_base;
	vmsplat_uint64_t	chpl_base;
	vmsplat_uint64_t	ckgn_base;
	vmsplat_uint64_t	i3c_apb_base;
	vmsplat_uint64_t	pcl_base;
};

/**
 * @brief Read Efuse
 *
 * This function reads the per cluster efuse and
 * fills a structure struct v2_fuses with various
 * values programmed in one time programmable efuse.
 *
 * NOTE: To get the read values in struct v2_fuses
 * instance use the helper function v2_spx_get_efuse
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 * @param[in] ckgn_base		CKGN Regbus Base Address
 *
 * @return enum vmsplat_error
 */
int v2_read_fuses(vmsplat_uint32_t cluster_id,
		  vmsplat_uint64_t ckgn_base);

/**
 * @brief Helper function to get initialized
 * struct v2_fuses instance with per
 * cluster efuse values
 *
 * NOTE: This function does not actually reads the Efuse
 * which is done via v2_read_fuses.
 * It must be called after calling v2_read_fuses.
 *
 * Various internal settings for feature selection,
 * Enabling/Disabling various blocks and limiting
 * the number of CPUs in PCL are programmed in efuse.
 * This function reads the efuse and returns these
 * configurations.
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 *
 * @return struct v2_fuses
 */
struct v2_fuses *v2_spx_get_efuse(vmsplat_uint32_t cluster_id);

/**
 * @brief Check with Chiplet BISR
 *
 * Check Built in Self Repair (BIST) for Chiplet
 * to proceed for cluster initialization
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 * @param[in] smrc_base		SMRC RegBus Base Address
 * @param[in] v2_mmio		Pointer to SATT configuration instance
 *
 * @return enum vmsplat_error
 */
int v2_check_bisr(vmsplat_uint32_t cluster_id, vmsplat_uint64_t smrc_base);

/**
 * @brief Configure V2 SATT
 *
 * SATT is System Address Translation Table which performs
 * mapping between 52-Bit System Address space to 24-Bit
 * SPRB Requester Address Space. SATT table contains
 * various RegBus regions base addresses for the system
 * address translation.
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 * @param[in] smrc_base		SMRC RegBus Base Address
 * @param[in] v2_mmio		Pointer to SATT configuration instance
 *
 * @return enum vmsplat_error
 */
int v2_config_satt(vmsplat_uint32_t cluster_id,
		   vmsplat_uint64_t smrc_base,
		   const struct v2_mmio_bases *v2_mmio);

/**
 * @brief Enable MTIME
 *
 * MTIME is a RISC-V Arch defined real-time counter
 * which increments at constant frequency.This function
 * enables the MTIME increment and configures the rate of
 * increment.
 *
 * NOTE: This function must be called after the v2_system_deassert
 * and before the v2_cluster_deassert.
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 * @param[in] lwic_base		LWIC RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int v2_enable_mtime(vmsplat_uint32_t cluster_id,
		    vmsplat_uint64_t lwic_base);

/**
 * @brief Configure PCL capability and bring its Non-Core
 * out of reset for further initialization and configuration
 * 
 * NOTE: This function must be called first before performing
 * any configuration or initialization of CPUs in Cluster
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 * @param[in] smrc_base		SMRC RegBus Base Address
 * @param[in] base_hartid	Base Hartid(RISC-V ARCH CPU ID) for Cluster
 * @param[in] cpu_vector	CPUs enabled in efuse
 * @param[in] num_ul3_slices	Number of UL3 Slices from efuse (num_harts-1)
 * @param[in] ul3_cap		Cache capacity of UL3(Cache lines)
 *
 * @return enum vmsplat_error
 */
int v2_system_deassert(vmsplat_uint32_t cluster_id,
		       vmsplat_uint64_t smrc_base,
		       vmsplat_uint64_t chib_base,
		       vmsplat_uint64_t lwic_base,
		       vmsplat_uint64_t pcrb_base,
		       vmsplat_uint32_t base_hartid,
		       vmsplat_uint16_t cpu_vector,
		       vmsplat_uint8_t num_l3m_slices,
		       vmsplat_uint8_t l3m_cap,
		       vmsplat_uint8_t cluster_ns,
		       vmsplat_uint16_t cluster_node_id0,
		       vmsplat_uint16_t cluster_node_id1);
/**
 * @brief Enable V2 Debug Module and configure
 * harts for debug
 * 
 * NOTE: This function must be called after performing
 * the system/cluster initialization, likely after the
 * v2_cluster_deassert but before v2_cluster_start
 *
 * @param[in] cluster_id	Cluster/Chiplet ID
 * @param[in] dbmd_base		Debug Module RegBus Base Address
 * @param[in] num_harts		Number of Harts available
 *
 * @return enum vmsplat_error
 */
int v2_dbmd_enable(vmsplat_uint32_t cluster_id,
		   vmsplat_uint64_t dbmd_base);

#endif /* __V2_SPX_INIT_H__ */
