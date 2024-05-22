/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#ifndef __VT1_PCL_INIT_H__
#define __VT1_PCL_INIT_H__

#include <vmsplat_types.h>
#include <vt1/vt1_pcl_hal.h>
#include <vt1/hw/vt1_cpu_mmr.h>
#include <vt1/hw/vt1_l3m_mmr.h>
#include <vt1/hw/vt1_sbr_mmr.h>
#include <vt1/hw/vt1_ics_mmr.h>

/* SBR Credits Register Value (ICS0 Only) */
#define VT1_SBR_SNPCREDITS0_CREDITS	3
#define VT1_SBR_DATCREDITS0_CREDITS	6
#define VT1_SBR_RSPCREDITS0_CREDITS	3

/* ICS0 Credits Register Value */
#define VT1_ICS_ICSREQCREDITS0_CREDITS	2
#define VT1_ICS_SBRREQCREDITS_CREDITS	3
#define VT1_ICS_SNPCREDITS0_CREDITS	2
#define VT1_ICS_DATCREDITS0_CREDITS	3
#define VT1_ICS_RSPCREDITS0_CREDITS	2

#define VT1_PCL_CAP_CONFIG_ARRAY_MAX	256

/**
 * PCL/CPU Capability Possible Values
 */
enum vt1_pcl_cap_value{
	VT1_PCL_CAP_DISABLE = 0,
	VT1_PCL_CAP_ENABLE = 1,
	VT1_PCL_CAP_VALUE_MAX_IDX,
};

/**
 * PCL/CPU Capability Array Indices
 */
enum vt1_pcl_cap_config_idx {
	VT1_PCL_CAP_AIA = 0,
	VT1_PCL_CAP_SVINVAL = 1,
	VT1_PCL_CAP_CBO = 2,
	VT1_PCL_CAP_ZABCS = 3,
	VT1_PCL_CAP_VSCONDOPS = 4,
	VT1_PCL_CAP_CACHE = 5,
	VT1_PCL_CAP_SNOOP = 6,
	VT1_PCL_CAP_CONFIG_MAX_IDX,
};

/**
 * PCL/CPU AMC Power Coefficients Array Indices
 *
 * Each CPU has 10 Power Cofficients and single
 * set of cofficients is applicable for all CPUs
 * in a Cluster identically.
 */
enum vt1_pcl_amcpwr_coeff_idx {
	/* FBlk predicted */
	VT1_PCL_AMCPWR_COEFF0 = 0,
	/* FBlk fetched from IL2 */
	VT1_PCL_AMCPWR_COEFF1 = 1,
	/* Ops decoded & dispatched */
	VT1_PCL_AMCPWR_COEFF2 = 2,
	/* IXU reg/br Op */
	VT1_PCL_AMCPWR_COEFF3 = 3,
	/* LSU Load Op */
	VT1_PCL_AMCPWR_COEFF4 = 4,
	/* LSU Store Op */
	VT1_PCL_AMCPWR_COEFF5 = 5,
	/* FXU reg/transfer Op */
	VT1_PCL_AMCPWR_COEFF6 = 6,
	/* IL2/DL2 outbound request and inbound snoops */
	VT1_PCL_AMCPWR_COEFF7 = 7,
	/* DL2 pipeline request processed */
	VT1_PCL_AMCPWR_COEFF8 = 8,
	/* TWE table-walk processing initiated */
	VT1_PCL_AMCPWR_COEFF9 = 9,
	VT1_PCL_AMCPWR_COEFF_MAX_IDX,
};

/**
 * PCL/CPU AMC Power Coefficients Array Indices
 * Each CPU has 10 Perf Cofficients and single
 * set of cofficients is applicable for all CPUs
 * in a Cluster identically.
 */
enum vt1_pcl_amcperf_coeff_idx {
	/* FBlk predicted */
	VT1_PCL_AMCPERF_COEFF0 = 0,
	/* Number of Ops decoded and dispatched */
	VT1_PCL_AMCPERF_COEFF1 = 1,
	/* Abort Initiated */
	VT1_PCL_AMCPERF_COEFF2 = 2,
	/* DL2 write-related pipeline request processed */
	VT1_PCL_AMCPERF_COEFF3 = 3,
	/* DL2 non-write related pipeline request processed */
	VT1_PCL_AMCPERF_COEFF4 = 4,
	/* TWE table-walk processing initiated */
	VT1_PCL_AMCPERF_COEFF5 = 5,
	/* DL2 cache fill from L3 and DL2 within cluster */
	VT1_PCL_AMCPERF_COEFF6 = 6,
	/* DL2 cache fill from memory outside of cluster */
	VT1_PCL_AMCPERF_COEFF7 = 7,
	/* IL2 cache fill from L3 or DL2 within cluster */
	VT1_PCL_AMCPERF_COEFF8 = 8,
	/* IL2 cache fill from memory outside of cluster */
	VT1_PCL_AMCPERF_COEFF9 = 9,
	VT1_PCL_AMCPERF_COEFF_MAX_IDX,
};

/* VT1 PCL init functions available for users */

/**
 * \defgroup VT1_PCL_INIT VT1 PCL Interface
 * @brief Functions implemented by VT1 PCL Software IP
 * for VT1 Cluster/CPU Reset and Initialization. These functions
 * need to be called from the PCL IP host firmware interface
 * @{
 */

/**
 * @brief Initialize the VT1 Cluster
 *
 * This function initialize VT1 Cluster with initializing
 * each CPU/Hart and their respective L3M RAM. It only enables
 * the CPU/Hart which are enabled in the provided cpu vector
 * from the efuse.
 *
 * After all enabled cpu initialization, it enables cluster
 * to participate in system level coherency. Based on the
 * cpu initialization status various cpumasks are populated
 * for the runtime tracking of active/inactive cpus.
 *
 * This function internally calls the vt1_cpu_init for all
 * enabled CPUs initialization and provides one interface to
 * initialize the complete processor cluster.
 *
 * @param[in] apcluster_idx		Cluster ID
 * @param[in] num_harts			Number of harts/cpus in the cluster
 * @param[in] pcrb_base			PCL RegBus Base Address
 * @param[in] cpu_vector		Bitmap containing enabled CPUs
 * @param[in] zstage_load_addr		Load address of zstage(zero-stage bootloader) image
 * @param[in] pcl_cap			Array of PCL/CPU capability config
 * @param[in] num_pmaregion		Number of PMA regions
 * @param[in] pmaregion			Array of memory regions PMA configuration
 * @param[in] enable_amc		Flag to enable AMC Unit (Core + NonCore)
 * @param[in] pwr_coeffs		Array of Power estimation coefficients
 * @param[in] pwr_sample_interval	Power estimation sampling interval(ms)
 * @param[in] perf_coeffs		Array of Performance benefit estimation coefficients
 * @param[in] perf_sample_interval	Performance benefit estimation sampling interval(ms)
 * @param[in] noncore_pwr_sample_interval CPU Non-Core Power benefit estimation sampling interval(ms)
 * @param[in] noncore_perf_sample_interval CPU Non-Core Performance benefit estimation sampling interval(ms)
 * @param[in] set_cpu_disable		Callback for setting cpu in disabled cpumask
 * @param[in] set_cpu_ready		Callback to perform post cpu init(if any) process before start
 * @param[in] check_cpu_disabled	Callback to check cpu in disabled cpumask
 *
 * @return enum vmsplat_error
 */
int vt1_cluster_deassert(vmsplat_uint32_t apcluster_idx,
			 vmsplat_uint32_t num_harts,
			 vmsplat_uint64_t pcrb_base,
			 vmsplat_uint16_t cpu_vector,
			 vmsplat_uint64_t zstage_load_addr,
			 vmsplat_uint8_t *pcl_cap,
			 vmsplat_uint32_t num_pmaregion,
			 const struct vmsplat_config_pmaregion *pmaregion,
			 vmsplat_uint8_t enable_amc,
			 vmsplat_uint32_t *pwr_coeffs,
			 vmsplat_uint32_t pwr_sample_interval,
			 vmsplat_uint32_t *perf_coeffs,
			 vmsplat_uint32_t perf_sample_interval,
			 vmsplat_uint32_t noncore_pwr_sample_interval,
			 vmsplat_uint32_t noncore_perf_sample_interval,
	void (*set_cpu_disable)(vmsplat_uint32_t apcluster_idx,
				vmsplat_uint32_t cpu),
	void (*set_cpu_ready)(vmsplat_uint32_t apcluster_idx,
			      vmsplat_uint32_t cpu),
	vmsplat_bool_t (*check_cpu_disabled)(vmsplat_uint32_t apcluster_idx,
					     vmsplat_uint32_t cpu));

/**
 * @brief Start the VT1 Cluster
 *
 * This function starts all the CPUs/Harts which were
 * successfully initialized in vt1_cluster_deassert.
 * CPUs which successfully starts are marked in online
 * cpumask. But if a CPU even after initialization fails
 * to start is marked in disabled cpumask and that CPU
 * will remain disabled in this boot cycle.
 *
 * @param[in] apcluster_idx		Cluster ID
 * @param[in] num_harts			Number of harts/cpus in the cluster
 * @param[in] pcrb_base			PCL RegBus Base Address
 * @param[in] cpu_vector		Bitmap of enabled CPUs
 * @param[in] set_cpu_disable		Callback for setting cpu in disabled cpumask
 * @param[in] set_cpu_online		Callback for setting cpu in online cpumask
 * @param[in] check_cpu_disabled	Callback to check cpu in disabled cpumask
 *
 * @return enum vmsplat_error
 */
int vt1_cluster_start(vmsplat_uint32_t apcluster_idx,
		      vmsplat_uint32_t num_harts,
		      vmsplat_uint64_t pcrb_base,
		      vmsplat_uint16_t cpu_vector,
	void (*set_cpu_disable)(vmsplat_uint32_t apcluster_idx,
				vmsplat_uint32_t cpu),
	void (*set_cpu_online)(vmsplat_uint32_t apcluster_idx,
			       vmsplat_uint32_t cpu),
	vmsplat_bool_t (*check_cpu_disabled)(vmsplat_uint32_t apcluster_idx,
					     vmsplat_uint32_t cpu));

/**
 * @brief Initialize a VT1 CPU.
 *
 * It configures the basic blocks inside a CPU like
 * CPU RAM structures, PMA configurations and Reset/NMI
 * vector address, etc.
 *
 * NOTE: This function does not start the CPU.
 * This must always be called before vt1_cpu_start
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(apcluster_idx)
 * @param[in] zstage_load_addr	Load address of zstage(zero-stage bootloader) image
 * @param[in] pcl_cap		Array of PCL/CPU capability config
 * @param[in] num_pmaregion	Number of PMA regions
 * @param[in] pmaregion		Pointer to an array of PMA details for memory regions
 * @param[in] pcrb_base		PCL RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int vt1_cpu_init(vmsplat_uint32_t apcluster_idx,
		 vmsplat_uint16_t cpu,
		 vmsplat_uint64_t zstage_load_addr,
		 vmsplat_uint8_t *pcl_cap,
		 vmsplat_uint32_t num_pmaregion,
		 const struct vmsplat_config_pmaregion *pmaregion,
		 vmsplat_uint64_t pcrb_base);

/**
 * @brief Reset and Initialize a VT1 CPU.
 *
 * This function first performs a CPU Hard Reset
 * and then initializes the CPU.
 *
 * NOTE: This function does not start the CPU.
 * This must always be called before vt1_cpu_start
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(apcluster_idx)
 * @param[in] zstage_load_addr	Load address of zstage(zero-stage bootloader) image
 * @param[in] pcl_cap		Array of PCL/CPU capability config
 * @param[in] num_pmaregion	Number of PMA regions
 * @param[in] pmaregion		Pointer to an array of PMA details for memory regions
 * @param[in] pcrb_base		PCL RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int vt1_cpu_reset_and_init(vmsplat_uint32_t apcluster_idx,
			   vmsplat_uint16_t cpu,
			   vmsplat_uint64_t zstage_load_addr,
			   vmsplat_uint8_t *pcl_cap,
			   vmsplat_uint32_t num_pmaregion,
			   const struct vmsplat_config_pmaregion *pmaregion,
			   vmsplat_uint64_t pcrb_base);

/**
 * @brief Start a VT1 CPU.
 *
 * Enables CPU to initiate the first instruction fetch
 * from the configured reset vector.
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(apcluster_idx)
 * @param[in] pcrb_base		PCL RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int vt1_cpu_start(vmsplat_uint32_t apcluster_idx,
		  vmsplat_uint32_t cpu,
		  vmsplat_uint64_t pcrb_base);

/**
 * @brief Configure AMC Unit of a VT1 CPU
 *
 * Each CPU has AMC(Activity Monitoring Counters) which is
 * a set of counters to provide activity information on
 * power estimation and performance benefit estimation of
 * a CPU. This function initializes the various coefficients
 * and sampling interval required by the AMC Unit.
 *
 * This function also configures performance benefit estimation
 * counter meant for L3M. No coefficient is required for this
 * counter only the sampling interval.
 *
 * @param[in] apcluster_idx		Cluster ID
 * @param[in] cpu			CPU ID local to cluster(apcluster_idx)
 * @param[in] pcrb_base			PCL RegBus Base Address
 * @param[in] pwr_coeffs		Array of Power estimation coefficients
 * @param[in] pwr_sample_interval	Power estimation sampling interval(ms)
 * @param[in] perf_coeffs		Array of Performance benefit estimation coefficients
 * @param[in] perf_sample_interval	Performance benefit estimation sampling interval(ms)
 * @param[in] noncore_perf_sample_interval CPU Non-Core Performance benefit estimation sampling interval(ms)
 *
 * @return enum vmsplat_error
 */
int vt1_cpu_configure_amc(vmsplat_uint32_t apcluster_idx,
			  vmsplat_uint32_t cpu,
			  vmsplat_uint64_t pcrb_base,
			  vmsplat_uint32_t *pwr_coeffs,
			  vmsplat_uint32_t pwr_sample_interval,
			  vmsplat_uint32_t *perf_coeffs,
			  vmsplat_uint32_t perf_sample_interval,
			  vmsplat_uint32_t noncore_perf_sample_interval);

/**
 * @brief Initialize L3M RAM for a VT1 CPU
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(apcluster_idx)
 * @param[in] pcrb_base		PCL RegBus Base Address
 * @param[in] pcl_cap		Array of PCL/CPU capability config
 *
 * @return enum vmsplat_error
 */
int vt1_cpu_l3m_ram_init(vmsplat_uint32_t apcluster_idx,
			 vmsplat_uint32_t cpu,
			 vmsplat_uint64_t pcrb_base,
			 vmsplat_uint8_t *pcl_cap);

/** @} */

/* In nano seconds - delay required for RAM Init Done */
#define VT1_RAMINIT_DELAY	100000

#define VT1_PMP_SHIFT		2
#define VT1_PMA_SHIFT		3
#define VT1_PMA_ENTRIES_MAX	48

/* PMACFG Register Encoding for Memory Types */
#define VT1_PMA_TYPE_WB		0x3FFB8F7F
#define VT1_PMA_TYPE_WC		0x3DC08F7F
#define VT1_PMA_TYPE_UC		0x31000F7E
#define VT1_PMA_TYPE_VACANT	0x30000000

/* ClusterID from mhartid encoding */
#define VT1_CLUSTERID_POS	4
#define VT1_CLUSTERID_WID	4
#define VT1_CLUSTERID_MSK	\
	(((_ULL(1) << VT1_CLUSTERID_WID) - _ULL(1)) <<VT1_CLUSTERID_POS)

#define VT1_MAX_CLUSTER		16

/**
 * VT1 RegBus Helpers and Macros
 */

/*
 * Function IDs
 * 0x0-0xF: FN0 - FN15
 */
#define REGBUS_FUNCID_FN0 0x00
#define REGBUS_FUNCID_FN1 0x01
#define REGBUS_FUNCID_FN2 0x02
#define REGBUS_FUNCID_FN3 0x03
#define REGBUS_FUNCID_FN4 0x04
#define REGBUS_FUNCID_FN5 0x05
#define REGBUS_FUNCID_FN6 0x06
#define REGBUS_FUNCID_FN7 0x07
#define REGBUS_FUNCID_FN8 0x08
#define REGBUS_FUNCID_FN9 0x09
#define REGBUS_FUNCID_FNA 0x0A
#define REGBUS_FUNCID_FNB 0x0B
#define REGBUS_FUNCID_FNC 0x0C
#define REGBUS_FUNCID_FND 0x0D
#define REGBUS_FUNCID_FNE 0x0E
#define REGBUS_FUNCID_FNF 0x0F

/*
 * Unit Type
 */
#define REGBUS_UNITTYPE_CPU 0x0
#define REGBUS_UNITTYPE_L3M 0x1
#define REGBUS_UNITTYPE_ICS 0x2

/*
 * CPU Unit Type UNIT-IDs
 * 0x00-0x0F: CPU0 - CPU15
 */
#define REGBUS_UNITID_CPU0 0x00
#define REGBUS_UNITID_CPU1 0x01
#define REGBUS_UNITID_CPU2 0x02
#define REGBUS_UNITID_CPU3 0x03
#define REGBUS_UNITID_CPU4 0x04
#define REGBUS_UNITID_CPU5 0x05
#define REGBUS_UNITID_CPU6 0x06
#define REGBUS_UNITID_CPU7 0x07
#define REGBUS_UNITID_CPU8 0x08
#define REGBUS_UNITID_CPU9 0x09
#define REGBUS_UNITID_CPUA 0x0A
#define REGBUS_UNITID_CPUB 0x0B
#define REGBUS_UNITID_CPUC 0x0C
#define REGBUS_UNITID_CPUD 0x0D
#define REGBUS_UNITID_CPUE 0x0E
#define REGBUS_UNITID_CPUF 0x0F

/*
 * L3M Unit Type UNIT-IDs
 * 0x10-0x1F: L3M0 - L3M15
 */
#define REGBUS_UNITID_L3M0 0x10
#define REGBUS_UNITID_L3M1 0x11
#define REGBUS_UNITID_L3M2 0x12
#define REGBUS_UNITID_L3M3 0x13
#define REGBUS_UNITID_L3M4 0x14
#define REGBUS_UNITID_L3M5 0x15
#define REGBUS_UNITID_L3M6 0x16
#define REGBUS_UNITID_L3M7 0x17
#define REGBUS_UNITID_L3M8 0x18
#define REGBUS_UNITID_L3M9 0x19
#define REGBUS_UNITID_L3MA 0x1A
#define REGBUS_UNITID_L3MB 0x1B
#define REGBUS_UNITID_L3MC 0x1C
#define REGBUS_UNITID_L3MD 0x1D
#define REGBUS_UNITID_L3ME 0x1E
#define REGBUS_UNITID_L3MF 0x1F

/*
 * ICS Unit Type UNIT-ID
 * 0x20-0x27: ICS0 - ICS7
 */
#define REGBUS_UNITID_ICS0 0x20
#define REGBUS_UNITID_ICS1 0x21
#define REGBUS_UNITID_ICS2 0x22
#define REGBUS_UNITID_ICS3 0x23
#define REGBUS_UNITID_ICS4 0x24
#define REGBUS_UNITID_ICS5 0x25
#define REGBUS_UNITID_ICS6 0x26
#define REGBUS_UNITID_ICS7 0x27

/*
 * SBR Unit Type UNIT-IDs
 * 0x28: SBR
 */
#define REGBUS_UNITID_SBR 0x28

/*
 *                 +-----------------------+
 *                 |                       |
 *                 |         CPU(n)        |
 *                 |                       |
 * INSTANCE(n)     +----------------^---+--+
 *                                  |   |
 *                 +---------+   +--+---v--+
 *                 |         +--->         |
 *                 |  L3M(n) |   |  ICR(n) |
 *                 |         <---+         |
 *                 +---------+   +--^--+---+
 *                                  |  |
 *                               +--+--v----+     +-------+
 *                               |          +----->       |
 *                               |          |     |  SBR  |
 *                               |  ICS(n)  |     |       |
 *                               |          <-----+       |
 *                               +--^--+----+     +-------+
 *                                  |  |
 *                 +---------+   +--+--v---+
 *                 |         +--->         |
 *                 |         |   |         |
 *                 | L3M(n+1)<---+ ICR(n+1)|
 *                 |         |   |         |
 *                 +---------+   +--^---+--+
 *                                  |   |
 * INSTANCE(n+1)   +----------------+---v--+
 *                 |                       |
 *                 |        CPU(n+1)       |
 *                 |                       |
 *                 +-----------------------+
 *
 */

/*
 * Get the Unit-ID for any Unit-Type (CPU, L3M, ICS) from its instance
 * CPUn has L3Mn which is served by ICSn but ICSn also serve another
 * CPU instance which is CPU(n+1) with L3M(n+1)
 *
 * Instance Number for CPU and L3M are same
 */
#define GET_UNITID(UNITTYPE, INSTANCE)						\
	({									\
		((UNITTYPE == REGBUS_UNITTYPE_CPU)?				\
		INSTANCE: ((UNITTYPE == REGBUS_UNITTYPE_L3M)?			\
		((0x1 << 4) + INSTANCE): ((0x2 << 4) + (INSTANCE >> 1))));	\
	})



/* Regbus Subregion 0 ID */
#define REGBUS_SR0_ID			(0)
/* Cluster Regbus Subregion0 ID Position */
#define REGBUS_SR0_SUBREGION_ID_POS	(22)
/* Cluster Regbus Function ID Position */
#define REGBUS_SR0_FUNCID_POS		(18)
/* Cluster Regbus Unit ID Position */
#define REGBUS_SR0_UNITID_POS		(12)

/* Regbus Subregion 0 ID */
#define REGBUS_SR1_ID			(1)
/* Cluster Regbus Subregion0 ID Position */
#define REGBUS_SR1_SUBREGION_ID_POS	(22)
/* Cluster Regbus Unit ID Position */
#define REGBUS_SR1_UNITID_POS		(16)
/* Cluster Regbus Function ID Position */
#define REGBUS_SR1_FUNCID_POS		(12)

/* CSRBus Page Identifier for MMR */
#define CSRBUS_PAGE_IDN_POS	(8)

/* CSRBus Function-ID for MMR */
#define CSRBUS_SR0_FUNCID_POS	(8)
#define CSRBUS_SR0_FUNCID_WID	(4)
#define CSRBUS_SR0_FUNCID_MASK \
	(((_ULL(1) << CSRBUS_SR0_FUNCID_WID) - _ULL(1)) << CSRBUS_SR0_FUNCID_POS)

/* CSRBus Register Offset for MMR */
#define CSRBUS_SR0_REGOFFSET_POS	(0)
#define CSRBUS_SR0_REGOFFSET_WID	(8)
#define CSRBUS_SR0_REGOFFSET_MASK \
	(((_ULL(1) << CSRBUS_SR0_REGOFFSET_WID) - _ULL(1)) << CSRBUS_SR0_REGOFFSET_POS)

/* CSRBus Function-ID for MMR */
#define CSRBUS_SR1_FUNCID_POS	(4)
#define CSRBUS_SR1_FUNCID_WID	(4)
#define CSRBUS_SR1_FUNCID_MASK \
	(((_ULL(1) << CSRBUS_SR1_FUNCID_WID) - _ULL(1)) << CSRBUS_SR1_FUNCID_POS)

/* CSRBus Register Offset for MMR */
#define CSRBUS_SR1_REGOFFSET_POS	(0)
#define CSRBUS_SR1_REGOFFSET_WID	(4)
#define CSRBUS_SR1_REGOFFSET_MASK \
	(((_ULL(1) << CSRBUS_SR1_REGOFFSET_WID) - _ULL(1)) << CSRBUS_SR1_REGOFFSET_POS)

/*
 * Get Function-ID from Offset in header file for a MMR
 * MMR Offset is the CSRBus address
 */
#define FUNCID_FROM_MMROFFSET(offset)					\
({									\
	((((offset) >> CSRBUS_PAGE_IDN_POS) == 0xF)?			\
	(((offset) & CSRBUS_SR1_FUNCID_MASK) >> CSRBUS_SR1_FUNCID_POS) :\
	(((offset) & CSRBUS_SR0_FUNCID_MASK) >> CSRBUS_SR0_FUNCID_POS));\
})

/*
 * Get Register Offset from Offset in header file for a MMR/
 * MMR Offset is the CSRBus address
 */
#define REGOFFSET_FROM_MMROFFSET(offset)				\
({									\
	((((offset) >> CSRBUS_PAGE_IDN_POS) == 0xF)?			\
	(((offset) & CSRBUS_SR1_REGOFFSET_MASK) >> CSRBUS_SR1_REGOFFSET_POS):\
	(((offset) & CSRBUS_SR0_REGOFFSET_MASK) >> CSRBUS_SR0_REGOFFSET_POS));\
})

/* L3M Page Identifier for MMR */
#define L3M_PAGE_IDN_POS	(10)

/* L3M Function-ID for MMR */
#define L3M_SR0_FUNCID_POS	(10)
#define L3M_SR0_FUNCID_WID	(4)
#define L3M_SR0_FUNCID_MASK \
	(((_ULL(1) << L3M_SR0_FUNCID_WID) - _ULL(1)) << L3M_SR0_FUNCID_POS)

/* L3M Register Offset for MMR */
#define L3M_SR0_REGOFFSET_POS	(0)
#define L3M_SR0_REGOFFSET_WID	(10)
#define L3M_SR0_REGOFFSET_MASK \
	(((_ULL(1) << L3M_SR0_REGOFFSET_WID) - _ULL(1)) << L3M_SR0_REGOFFSET_POS)

/*
 * Get Function-ID from Offset in header file for a MMR
 * MMR Offset is the L3M address
 */
#define L3M_FUNCID_FROM_MMROFFSET(offset)					\
({									\
	((offset & L3M_SR0_FUNCID_MASK) >> L3M_SR0_FUNCID_POS);	\
})

/*
 * Get Register Offset from Offset in header file for a MMR/
 * MMR Offset is the L3M address
 */
#define L3M_REGOFFSET_FROM_MMROFFSET(offset)					\
({										\
	((offset & L3M_SR0_REGOFFSET_MASK) >> L3M_SR0_REGOFFSET_POS);		\
})

/* Subregion0 Function-Unit Offset
 * This is the offset of the 4-KB Page assigned to a Function-Unit pair.
 * Subregion0 Function-Unit Offset
 * This is the offset of the 4-KB Page assigned to a Function-Unit pair.
 * Each page implements the 256  4-Byte Registers.
 * */
#define PCL_REGBUS_SR0_FUNCUNIT_OFFSET(funcid, unitid) \
			((((REGBUS_SR0_ID) << (REGBUS_SR0_SUBREGION_ID_POS))| \
			((funcid) << (REGBUS_SR0_FUNCID_POS)))| \
			((unitid) << (REGBUS_SR0_UNITID_POS)))


/* Subregion1 Function-Unit Offset
 * This is the offset of the 4-KB Page assigned to a Function-Unit pair.
 * Each page implements the 256  4-Byte Registers.
 * */
#define PCL_REGBUS_SR1_FUNCUNIT_OFFSET(funcid, unitid) \
			((((REGBUS_SR1_ID) << (REGBUS_SR1_SUBREGION_ID_POS))| \
			((funcid) << (REGBUS_SR1_FUNCID_POS)))| \
			((unitid) << (REGBUS_SR1_UNITID_POS)))

/*
 * CPU MMR register location
 */
#define CPU_MMR(pcrb_base, cpuinst, regoff)					\
	({										\
		pcrb_base +								\
		VT1_REG_32(PCL_REGBUS_SR0_FUNCUNIT_OFFSET(FUNCID_FROM_MMROFFSET(regoff),\
						   GET_UNITID(REGBUS_UNITTYPE_CPU,	\
							      cpuinst)),		\
			   REGOFFSET_FROM_MMROFFSET(regoff));				\
	})

/*
 * L3M MMR register location
 */
#define L3M_MMR(pcrb_base, cpuinst, regoff)						\
	({										\
		pcrb_base +								\
		VT1_REG_32(PCL_REGBUS_SR0_FUNCUNIT_OFFSET(L3M_FUNCID_FROM_MMROFFSET(regoff),\
						   GET_UNITID(REGBUS_UNITTYPE_L3M,	\
							      cpuinst)),		\
			   L3M_REGOFFSET_FROM_MMROFFSET(regoff));			\
	})

/*
 * ICS MMR register location
 */
#define ICS_MMR(pcrb_base, cpuinst, regoff)						\
	({										\
		pcrb_base +								\
		VT1_REG_32(PCL_REGBUS_SR0_FUNCUNIT_OFFSET(FUNCID_FROM_MMROFFSET(regoff),\
						   GET_UNITID(REGBUS_UNITTYPE_ICS,	\
							      cpuinst)),		\
			   REGOFFSET_FROM_MMROFFSET(regoff));				\
	})

/*
 * SBR MMR register location
 */
#define SBR_MMR(pcrb_base, regoff)							\
	({										\
		pcrb_base +								\
		VT1_REG_32(PCL_REGBUS_SR0_FUNCUNIT_OFFSET(FUNCID_FROM_MMROFFSET(regoff),\
						   REGBUS_UNITID_SBR),			\
			   REGOFFSET_FROM_MMROFFSET(regoff));				\
	})

/*
 * Cluter Register Read Modify Write Helper
 *
 * @cluster: Cluster ID
 * @addr: Address of Register/MMR
 * @mask: Mask for complete register or any field in that register
 * @pos: Position of Field or 0 in case of complete register
 * @data: Data to write in register
 *
 * This macro calls the SPRB read and write calls. Depending on I3C
 * or MMIO SPRB active during initialization time the SPRB will
 * route the read/write calls to correct implementation.
 *
 * MMIO SPRB read/write calls dont need cluster id but address should
 * be system address for that register/mmr.
 * In case of I3C SPRB the read/write calls require cluster id and
 * addr represents the offset for that register in particular cluster
 * address space.
 *
 * To not worry about the address or offset for any register use the
 * address/offset generation macros below which takes care of this
 * as per the current activated SPRB.
 */
#define CLUSTER_REG_RMW(cluster, addr, mask, pos, data) ({			\
	int __rc = 0;								\
	do {									\
		vmsplat_uint32_t __val = 0;					\
		__rc = vt1_read(cluster, addr, &__val);			\
		if (!__rc) {							\
			VT1_SET_FIELD(__val, mask, pos, data);		\
			__rc = vt1_write(cluster, addr, __val);		\
			if (__rc) {						\
				vt1_error("reg write fail-addr:%llx cluster:%u",\
						addr, cluster);			\
			}							\
		} else	{							\
			vt1_error("reg read fail-addr:%llx cluster:%u"		\
					, addr, cluster);			\
		}								\
	} while(0);								\
	__rc;									\
})

#endif /* __VT1_PCL_INIT_H__ */
