/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 */

#ifndef __V2_PCL_INIT_H__
#define __V2_PCL_INIT_H__

#include <vmsplat_types.h>
#include <vt2_pcl_hal.h>
#include <hw/vt2_cpu_mmr.h>
#include <hw/vt2_l3m_mmr.h>
#include <hw/vt2_icp_clock_mmr.h>
#include <hw/vt2_icp_rnf_mmr.h>

/* In nano seconds - delay required for RAM Init Done */
#define V2_RAMINIT_DELAY		100000

/* Delay between two consequtive read for HardResetDone bit */
#define V2_HARDRESET_DONE_DELAY		100000


#define V2_PMP_SHIFT			2
#define V2_PMA_SHIFT			3
#define V2_PMA_ENTRIES_MAX		24

/* PMACFG Register Encoding for Memory Types */
#define V2_PMA_TYPE_WB			0x3FFB8F7F
#define V2_PMA_TYPE_WC			0x3DC08F7F
#define V2_PMA_TYPE_UC			0x31000F7E
#define V2_PMA_TYPE_VACANT		0x30000000

/* ClusterID from mhartid encoding */
#define V2_CLUSTERID_POS		5
#define V2_CLUSTERID_WID		4
#define V2_CLUSTERID_MSK	\
	(((_ULL(1) << V2_CLUSTERID_WID) - _ULL(1)) <<	V2_CLUSTERID_POS)

#define V2_MAX_CLUSTER			16

#define V2_PMA_REG_STRIDE		0x10
/** Number of CPU RAS banks */
#define V2_CPU_RASBANK_NUM		15

/** Number of L3M RAS banks */
#define V2_L3M_RASBANK_NUM		4

/** CPU RAS bank registers stride */
#define V2_CPU_RASBANK_REG_STRIDE	0x40

/** L3M RAS bank registers stride */
#define V2_L3M_RASBANK_REG_STRIDE	0x40

/** CPU ISA Extension configurations */
struct v2_config_cpu_isa_extension {
	vmsplat_bool_t disable_vector;
	vmsplat_bool_t disable_zvk;
	vmsplat_bool_t disable_zvfhmin;
	vmsplat_bool_t disable_zvfbfmin;
	vmsplat_bool_t disable_zvfbfwma;
	vmsplat_bool_t disable_xvwmatmul;
	vmsplat_bool_t disable_zfa;
	vmsplat_bool_t disable_zfhmin;
	vmsplat_bool_t disable_zfbfmin;
	vmsplat_bool_t disable_zicond;
	vmsplat_bool_t disable_zawrs;
	vmsplat_bool_t disable_zvkned;
	vmsplat_bool_t disable_xvwadaccu;
};

/** CPU config registers for uArch tweaks and chicken bits */
struct v2_config_cpu_capability {
	vmsplat_uint32_t pruconfig1;
	vmsplat_uint32_t ifuconfig;
	vmsplat_uint32_t decconfig1;
	vmsplat_uint32_t decconfig2;
	vmsplat_uint32_t declvpconfig;
	vmsplat_uint32_t gpcconfig;
	vmsplat_uint32_t ixuconfig;
	vmsplat_uint32_t fxuconfig;
	vmsplat_uint32_t vxuconfig;
	vmsplat_uint32_t lsuconfig;
	vmsplat_uint32_t l2mmemutil;
	vmsplat_uint32_t l2mconfig;
	vmsplat_uint32_t l2msmptcpconfig;
	vmsplat_uint32_t tweconfig;
	vmsplat_uint32_t pruconfig2;
	vmsplat_uint32_t pruconfig3;
	vmsplat_uint32_t afeconfig;
	vmsplat_uint32_t pruconfig4;
	vmsplat_uint32_t lsuconfig1;
	vmsplat_uint32_t cpuramctl;
	vmsplat_uint32_t l2mconfig2;
};

/** L3M config registers for uArch tweaks and chicken bits */
struct v2_config_l3m_capability {
	vmsplat_uint32_t	ul3config1;
	vmsplat_uint32_t	ul3config2;
	vmsplat_uint32_t	ul3config3;
	vmsplat_uint32_t	ul3config4;
};

struct v2_config_pcl_capability {
	struct v2_config_cpu_isa_extension	cpu_isa_config;
	struct v2_config_cpu_capability		cpu_uarch_config;
	struct v2_config_l3m_capability		l3m_uarch_config;
};

/**
 * V2 RAS Signal Priority as defined by the RERI
 * The priorities are common for both CPU and L3M
 * RAS signals
 */
enum v2_ras_signal_priority {
	V2_RAS_SIGNAL_PRIO_DISABLE,
	V2_RAS_SIGNAL_PRIO_LOW,
	V2_RAS_SIGNAL_PRIO_HIGH,
	V2_RAS_SIGNAL_PRIO_PLATFORM,
	V2_RAS_SIGNAL_PRIO_MAX_IDX,
};

enum v2_cluster_impid_types {
	/** Ventana 16x16x2 VT2-Loki, A0 stepping */
	V2_CLUSTER_IMPID_1 = 1,
	/**
	 * TODO: This is not defined yet and its a placeholder for
	 * a design which has 4 RN-F interfaces
	 */
	V2_CLUSTER_IMPID_2 = 2,
};

/**
 * struct v2_config_cpu_tempsensor - PCL Temp Sensor(TS) Config
 * @mode:		IP operation mode - Conversion(0) or Direct(8)
 * @temp_max_cel:	Max Temp Threshold which when
 *			crossed will generate the interrupt.
 * @resolution:		Temp sensor resolution - 12,10 or 8 (bit)
 * @y_coeff:		Calibrated Y cofficient.
 * @k_coeff:		Calibrated K cofficient.
 */
struct v2_config_cpu_tempsensor {
	vmsplat_uint8_t		mode;
	vmsplat_int32_t		temp_max_cel;
	vmsplat_uint32_t	resolution;
	float			y_coeff;
	float			k_coeff;
};

/**
 * PCL/CPU AMC Power Coefficients Array Indices
 *
 * Each CPU has 10 Power Cofficients and single
 * set of cofficients is applicable for all CPUs
 * in a Cluster identically.
 */
enum v2_pcl_amcpwr_coeff_idx {
	/* FBlk predicted */
	V2_PCL_AMCPWR_COEFF0,
	/* FBlk fetched from IL2 */
	V2_PCL_AMCPWR_COEFF1,
	/* Ops decoded & dispatched */
	V2_PCL_AMCPWR_COEFF2,
	/* IXU reg/br Op */
	V2_PCL_AMCPWR_COEFF3,
	/* LSU Load Op */
	V2_PCL_AMCPWR_COEFF4,
	/* LSU Store Op */
	V2_PCL_AMCPWR_COEFF5,
	/* FXU reg/transfer Op */
	V2_PCL_AMCPWR_COEFF6,
	/* IL2/DL2 outbound request and inbound snoops */
	V2_PCL_AMCPWR_COEFF7,
	/* DL2 pipeline request processed */
	V2_PCL_AMCPWR_COEFF8,
	/* TWE table-walk processing initiated */
	V2_PCL_AMCPWR_COEFF9,
	V2_PCL_AMCPWR_COEFF_MAX_IDX,
};

/**
 * PCL/CPU AMC Power Coefficients Array Indices
 * Each CPU has 10 Perf Cofficients and single
 * set of cofficients is applicable for all CPUs
 * in a Cluster identically.
 */
enum v2_pcl_amcperf_coeff_idx {
	/* FBlk predicted */
	V2_PCL_AMCPERF_COEFF0,
	/* Number of Ops decoded and dispatched */
	V2_PCL_AMCPERF_COEFF1,
	/* Abort Initiated */
	V2_PCL_AMCPERF_COEFF2,
	/* DL2 write-related pipeline request processed */
	V2_PCL_AMCPERF_COEFF3,
	/* DL2 non-write related pipeline request processed */
	V2_PCL_AMCPERF_COEFF4,
	/* TWE table-walk processing initiated */
	V2_PCL_AMCPERF_COEFF5,
	/* DL2 cache fill from L3 and DL2 within cluster */
	V2_PCL_AMCPERF_COEFF6,
	/* DL2 cache fill from memory outside of cluster */
	V2_PCL_AMCPERF_COEFF7,
	/* IL2 cache fill from L3 or DL2 within cluster */
	V2_PCL_AMCPERF_COEFF8,
	/* IL2 cache fill from memory outside of cluster */
	V2_PCL_AMCPERF_COEFF9,
	V2_PCL_AMCPERF_COEFF_MAX_IDX,
};

/**
 * V2 AMC Configuration for CPU and Non-Core Activity
 * Counters.
 */
struct v2_config_cpu_amc {
	vmsplat_uint32_t pwr_coeffs[V2_PCL_AMCPWR_COEFF_MAX_IDX];
	vmsplat_uint32_t pwr_sample_interval;
	vmsplat_uint32_t perf_coeffs[V2_PCL_AMCPERF_COEFF_MAX_IDX];
	vmsplat_uint32_t perf_sample_interval;
	vmsplat_uint32_t noncore_pwr_sample_interval;
	vmsplat_uint32_t noncore_perf_sample_interval;
};

/* V2 PCL init functions available for users */

/**
 * @brief Initialize the V2 Cluster
 *
 * This function initialize V2 Cluster with initializing
 * each CPU/Hart and their respective L3M RAM. It only enables
 * the CPU/Hart which are enabled in the provided cpu vector
 * from the efuse.
 *
 * After all enabled cpu initialization, it enables cluster
 * to participate in system level coherency. Based on the
 * cpu initialization status various cpumasks are populated
 * for the runtime tracking of active/inactive cpus.
 *
 * This function internally calls the v2_cpu_init for all
 * enabled CPUs initialization and provides one interface to
 * initialize the complete processor cluster.
 *
 * @param[in] cluster_id		Cluster ID
 * @param[in] num_harts			Number of harts/cpus in the cluster
 * @param[in] pcrb_base			PCL RegBus Base Address
 * @param[in] cpu_vector		Bitmap containing enabled CPUs
 * @param[in] zstage_load_addr		Load address of zstage(zero-stage bootloader) image
 * @param[in] cfg_pclcap		Cluster(CPU and L3M) chicken configs and uarch tweaks
 * @param[in] num_pmaregion		Number of PMA regions
 * @param[in] cfg_pmaregion		Array of memory regions PMA configuration
 * @param[in] enable_amc		Enable and configure the CPU AMC unit
 * @param[in] cfg_amc			Array of AMC config parameters
 *
 * @return enum vmsplat_error
 */
int v2_cluster_deassert(vmsplat_uint32_t cluster_id,
			vmsplat_uint32_t num_harts,
			vmsplat_uint64_t pcrb_base,
			vmsplat_uint16_t cpu_vector,
		        vmsplat_uint8_t num_l3m_slices,
			vmsplat_uint64_t zstage_load_addr,
			const struct v2_config_pcl_capability *cfg_pclcap,
			vmsplat_uint32_t num_pmaregion,
			const struct vmsplat_config_pmaregion *cfg_pmaregion,
		 	vmsplat_uint8_t enable_amc,
			struct v2_config_cpu_amc *cfg_amc);

/**
 * @brief Start the V2 Cluster
 *
 * This function starts all the CPUs/Harts which were
 * successfully initialized in v2_cluster_deassert.
 * CPUs which successfully starts are marked in online
 * cpumask. But if a CPU even after initialization fails
 * to start is marked in disabled cpumask and that CPU
 * will remain disabled in this boot cycle.
 *
 * @param[in] cluster_id		Cluster ID
 * @param[in] num_harts			Number of harts/cpus in the cluster
 * @param[in] pcrb_base			PCL RegBus Base Address
 * @param[in] cpu_vector		Bitmap of enabled CPUs
 *
 * @return enum vmsplat_error
 */
int v2_cluster_start(vmsplat_uint32_t cluster_id,
		     vmsplat_uint32_t num_harts,
		     vmsplat_uint64_t pcrb_base,
		     vmsplat_uint16_t cpu_vector);

/**
 * @brief Initialize a V2 CPU.
 *
 * It configures the basic blocks inside a CPU like
 * CPU RAM structures, PMA configurations and Reset/NMI
 * vector address, etc.
 *
 * NOTE: This function does not start the CPU.
 * This must always be called before v2_cpu_start
 *
 * @param[in] cluster_id	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(cluster_id)
 * @param[in] zstage_load_addr	Load address of zstage(zero-stage bootloader) image
 * @param[in] pcl_cap		Array of PCL/CPU capability config
 * @param[in] num_pmaregion	Number of PMA regions
 * @param[in] pmaregion		Pointer to an array of PMA details for memory regions
 * @param[in] pcrb_base		PCL RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int v2_cpu_init(vmsplat_uint32_t cluster_id,
		vmsplat_uint16_t cpu,
		vmsplat_uint64_t zstage_load_addr,
		const struct v2_config_cpu_isa_extension *cfg_cpu_isa,
		const struct v2_config_cpu_capability *cfg_cpu_uarch,
		vmsplat_uint32_t num_pmaregion,
		const struct vmsplat_config_pmaregion *cfg_pmaregion,
		vmsplat_uint64_t pcrb_base);

/**
 * @brief Reset and Initialize a V2 CPU.
 *
 * This function first performs a CPU Hard Reset
 * and then initializes the CPU.
 *
 * NOTE: This function does not start the CPU.
 * This must always be called before v2_cpu_start
 *
 * @param[in] cluster_id	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(cluster_id)
 * @param[in] zstage_load_addr	Load address of zstage(zero-stage bootloader) image
 * @param[in] pcl_cap		Array of PCL/CPU capability config
 * @param[in] num_pmaregion	Number of PMA regions
 * @param[in] pmaregion		Pointer to an array of PMA details for memory regions
 * @param[in] pcrb_base		PCL RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int v2_cpu_reset_and_init(vmsplat_uint32_t cluster_id,
			  vmsplat_uint16_t cpu,
			  vmsplat_uint64_t zstage_load_addr,
			  vmsplat_uint8_t *pcl_cap,
			  vmsplat_uint32_t num_pmaregion,
			  const struct vmsplat_config_pmaregion *pmaregion,
			  vmsplat_uint64_t pcrb_base);

/**
 * @brief Start a V2 CPU.
 *
 * Enables CPU to initiate the first instruction fetch
 * from the configured reset vector.
 *
 * @param[in] cluster_id	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(cluster_id)
 * @param[in] pcrb_base		PCL RegBus Base Address
 *
 * @return enum vmsplat_error
 */
int v2_cpu_start(vmsplat_uint32_t cluster_id,
		 vmsplat_uint32_t cpu,
		 vmsplat_uint64_t pcrb_base);

/**
 * @brief Initialize L3M RAM for a V2 CPU
 *
 * @param[in] cluster_id	Cluster ID
 * @param[in] cpu		CPU ID local to cluster(cluster_id)
 * @param[in] pcrb_base		PCL RegBus Base Address
 * @param[in] pcl_cap		Array of PCL/CPU capability config
 *
 * @return enum vmsplat_error
 */
int v2_l3m_init(vmsplat_uint32_t cluster_id,
		vmsplat_uint32_t cpu,
		vmsplat_uint64_t pcrb_base,
		const struct v2_config_l3m_capability *l3m_uarch_config);


int v2_cpu_ras_dump_state(vmsplat_uint32_t cluster_id,
			  vmsplat_uint32_t cpu,
			   vmsplat_uint64_t pcrb_base);
int v2_l3m_ras_dump_state(vmsplat_uint32_t cluster_id,
			  vmsplat_uint32_t cpu,
			  vmsplat_uint64_t pcrb_base);

/** @} */

/****************************************************************************/

/**
 * Below are the V2 PCL Regbus address conversion helpers.
 * Nothing must be changed below.
 */


enum cpu_unitid {
	CPU0_UNITID = 0x0,
	CPU1_UNITID = 0x1,
	CPU2_UNITID = 0x2,
	CPU3_UNITID = 0x3,
	CPU4_UNITID = 0x4,
	CPU5_UNITID = 0x5,
	CPU6_UNITID = 0x6,
	CPU7_UNITID = 0x7,
	CPU8_UNITID = 0x8,
	CPU9_UNITID = 0x9,
	CPU10_UNITID = 0xA,
	CPU11_UNITID = 0xB,
	CPU12_UNITID = 0xC,
	CPU13_UNITID = 0xD,
	CPU14_UNITID = 0xE,
	CPU15_UNITID = 0xF,
	CPU16_UNITID = 0x10,
	CPU17_UNITID = 0x11,
	CPU18_UNITID = 0x12,
	CPU19_UNITID = 0x13,
	CPU20_UNITID = 0x14,
	CPU21_UNITID = 0x15,
	CPU22_UNITID = 0x16,
	CPU23_UNITID = 0x17,
	CPU24_UNITID = 0x18,
	CPU25_UNITID = 0x19,
	CPU26_UNITID = 0x1A,
	CPU27_UNITID = 0x1B,
	CPU28_UNITID = 0x1C,
	CPU29_UNITID = 0x1D,
	CPU30_UNITID = 0x1E,
	CPU31_UNITID = 0x1F,
};

enum l3m_unitid {
	L3M0_UNITID = 0x20,
	L3M1_UNITID = 0x21,
	L3M2_UNITID = 0x22,
	L3M3_UNITID = 0x23,
	L3M4_UNITID = 0x24,
	L3M5_UNITID = 0x25,
	L3M6_UNITID = 0x26,
	L3M7_UNITID = 0x27,
	L3M8_UNITID = 0x28,
	L3M9_UNITID = 0x29,
	L3M10_UNITID = 0x2A,
	L3M11_UNITID = 0x2B,
	L3M12_UNITID = 0x2C,
	L3M13_UNITID = 0x2D,
	L3M14_UNITID = 0x2E,
	L3M15_UNITID = 0x2F,
	L3M16_UNITID = 0x30,
	L3M17_UNITID = 0x31,
	L3M18_UNITID = 0x32,
	L3M19_UNITID = 0x33,
	L3M20_UNITID = 0x34,
	L3M21_UNITID = 0x35,
	L3M22_UNITID = 0x36,
	L3M23_UNITID = 0x37,
	L3M24_UNITID = 0x38,
	L3M25_UNITID = 0x39,
	L3M26_UNITID = 0x3A,
	L3M27_UNITID = 0x3B,
	L3M28_UNITID = 0x3C,
	L3M29_UNITID = 0x3D,
	L3M30_UNITID = 0x3E,
	L3M31_UNITID = 0x3F,
};

enum icc_icp_unitid {
	ICC0_UNITID = 0x40,
	ICC1_UNITID = 0x41,
	ICC2_UNITID = 0x42,
	ICC3_UNITID = 0x43,
	ICP4_UNITID = 0x44,
	ICP5_UNITID = 0x45,
	ICP6_UNITID = 0x46,
	ICP7_UNITID = 0x47,
	ICC8_UNITID = 0x48,
	ICC9_UNITID = 0x49,
	ICC10_UNITID = 0x4A,
	ICC11_UNITID = 0x4B,
	ICP12_UNITID = 0x4C,
	ICP13_UNITID = 0x4D,
	ICP14_UNITID = 0x4E,
	ICP15_UNITID = 0x4F,
	ICC16_UNITID = 0x50,
	ICC17_UNITID = 0x51,
	ICC18_UNITID = 0x52,
	ICC19_UNITID = 0x53,
	ICP20_UNITID = 0x54,
	ICP21_UNITID = 0x55,
	ICP22_UNITID = 0x56,
	ICP23_UNITID = 0x57,
	ICC24_UNITID = 0x58,
	ICC25_UNITID = 0x59,
	ICC26_UNITID = 0x5A,
	ICC27_UNITID = 0x5B,
	ICP28_UNITID = 0x5C,
	ICP29_UNITID = 0x5D,
	ICP30_UNITID = 0x5E,
	ICP31_UNITID = 0x5F,
};

enum misc_unitid {
	MISC0_UNITID = 0x60,
	MISC1_UNITID = 0x61,
	MISC2_UNITID = 0x62,
	MISC3_UNITID = 0x63,
};

#define GET_FIELD(__var, __mask, __shift)	\
			(((__var) & (__mask)) >> (__shift))

/** Generate mask from field position and width */
#define MASK(__wid, __pos)	(((_ULL(1) << __wid) - _ULL(1)) << __pos)

/**
 ********************* CPU MMR Address Calculation Macros *******************
 */

#define CPU_UNIT_ROW_POS	3
#define CPU_UNIT_ROW_WID	2
#define CPU_UNIT_ROW_MSK	MASK(CPU_UNIT_ROW_WID, CPU_UNIT_ROW_POS)

/** Get the Row of CPU Unit */
#define ROW_CPU(__cpuuid)	GET_FIELD(__cpuuid, CPU_UNIT_ROW_MSK, CPU_UNIT_ROW_POS)

/** Function ID of register in register offset */
#define REG_CPU_FUNCID_POS		(8)
#define REG_CPU_FUNCID_WID		(4)
#define REG_CPU_FUNCID_MSK		\
			MASK(REG_CPU_FUNCID_WID, REG_CPU_FUNCID_POS)

/** Byte offset of register in register offset */
#define REG_CPU_BYTE_OFFSET_POS		(0)
#define REG_CPU_BYTE_OFFSET_WID		(8)
#define REG_CPU_BYTE_OFFSET_MSK		\
			MASK(REG_CPU_BYTE_OFFSET_WID, REG_CPU_BYTE_OFFSET_POS)

/** Get register function id from register offset */
#define REG_CPU_FUNCID(__regoff)					\
		GET_FIELD(__regoff, REG_CPU_FUNCID_MSK, REG_CPU_FUNCID_POS)

/**
 * Get register word offset from register offset
 *
 * word_offset = byte_offset << 2
 */
#define REG_CPU_WORD_OFFSET(__regoff)					\
(GET_FIELD(__regoff, REG_CPU_BYTE_OFFSET_MSK, REG_CPU_BYTE_OFFSET_POS) << 2)

/**
 ******************** L3M MMR Address Calculation Macros *******************
 */

/** Function ID of register in register offset */
#define REG_L3M_FUNCID_POS		(10)
#define REG_L3M_FUNCID_WID		(4)
#define REG_L3M_FUNCID_MSK		\
			MASK(REG_L3M_FUNCID_WID, REG_L3M_FUNCID_POS)

/** Byte offset of register in register offset */
#define REG_L3M_BYTE_OFFSET_POS		(0)
#define REG_L3M_BYTE_OFFSET_WID		(10)
#define REG_L3M_BYTE_OFFSET_MSK		\
			MASK(REG_L3M_BYTE_OFFSET_WID, REG_L3M_BYTE_OFFSET_POS)

/** Get register function id from register offset */
#define REG_L3M_FUNCID(__regoff)					\
		GET_FIELD(__regoff, REG_L3M_FUNCID_MSK, REG_L3M_FUNCID_POS)

/**
 * Get register word offset from register offset
 *
 * word_offset = byte_offset << 2
 */
#define REG_L3M_WORD_OFFSET(__regoff)					\
(GET_FIELD(__regoff, REG_L3M_BYTE_OFFSET_MSK, REG_L3M_BYTE_OFFSET_POS) << 2)

/**
 * Convert CPU unitid to L3M unitid
 * Get the L3M unitid which is connected to the CPU
 *
 * CPU{0x00, 0x01, 0x02 ... 0x1F} -> L3M {0x20, 0x21, 0x22 ... 0x3F}
 */
#define CPU_TO_L3M_UNITID(__cpuuid)		(L3M0_UNITID +  __cpuuid)

/**
 ******************** ICC/ICP MMR Address Calculation Macros ****************
 */
/** Function ID of register in register offset */
#define REG_ICC_ICP_CLOCKING_FUNCID_POS		(10)
#define REG_ICC_ICP_CLOCKING_FUNCID_WID		(4)
#define REG_ICC_ICP_CLOCKING_FUNCID_MSK		\
	MASK(REG_ICC_ICP_CLOCKING_FUNCID_WID, REG_ICC_ICP_CLOCKING_FUNCID_POS)

/** Byte offset of register in register offset */
#define REG_ICC_ICP_CLOCKING_BYTE_OFFSET_POS	(0)
#define REG_ICC_ICP_CLOCKING_BYTE_OFFSET_WID	(10)
#define REG_ICC_ICP_CLOCKING_BYTE_OFFSET_MSK	\
	MASK(REG_ICC_ICP_CLOCKING_BYTE_OFFSET_WID, REG_ICC_ICP_CLOCKING_BYTE_OFFSET_POS)

/** Get register function id from register offset */
#define REG_ICC_ICP_CLOCKING_FUNCID(__regoff)					\
	GET_FIELD(__regoff, REG_ICC_ICP_CLOCKING_FUNCID_MSK, REG_ICC_ICP_CLOCKING_FUNCID_POS)

/**
 * Get register word offset from register offset
 *
 * word_offset = byte_offset << 2
 */
#define REG_ICC_ICP_CLOCKING_WORD_OFFSET(__regoff)				\
(GET_FIELD(__regoff, REG_ICC_ICP_CLOCKING_BYTE_OFFSET_MSK, REG_ICC_ICP_CLOCKING_BYTE_OFFSET_POS) << 2)

/**
 * Convert CPU unitid to ICC/ICP unitid
 * Get the ICC or ICP unitid which is connected to the CPU
 *
 * CPU {0x00, 0x01, 0x02 ... 0x1F} -> ICC/ICP {0x40, 0x41, 0x42 ... 0x5F}
 */
#define CPU_TO_ICC_ICP_CLOCKING_UNITID(__cpuuid)	(ICC0_UNITID +  __cpuuid)

/**
 ******************** MISC I/C MMR Address Calculation Macros ***************
 *
 * MISC I/C block contains the RN-F register blocks
 * and a RN-F register block services a single ROW of 8 CPUs
 * Total 4 RN-Fs for 32 CPUs and 2 RN-F for 16 CPUs in 8N
 * design configuration
 */
/** Function ID of register in register offset */
#define REG_ICP_RNF_FUNCID_POS		(10)
#define REG_ICP_RNF_FUNCID_WID		(4)
#define REG_ICP_RNF_FUNCID_MSK		\
			MASK(REG_ICP_RNF_FUNCID_WID, REG_ICP_RNF_FUNCID_POS)

/** Byte offset of register in register offset */
#define REG_ICP_RNF_BYTE_OFFSET_POS	(0)
#define REG_ICP_RNF_BYTE_OFFSET_WID	(10)
#define REG_ICP_RNF_BYTE_OFFSET_MSK	\
		MASK(REG_ICP_RNF_BYTE_OFFSET_WID, REG_ICP_RNF_BYTE_OFFSET_POS)

/** Get register function id from register offset */
#define REG_ICP_RNF_FUNCID(__regoff)					\
	GET_FIELD(__regoff, REG_ICP_RNF_FUNCID_MSK, REG_ICP_RNF_FUNCID_POS)

/**
 * Get register word offset from register offset
 *
 * word_offset = byte_offset << 2
 */
#define REG_ICP_RNF_WORD_OFFSET(__regoff)				\
(GET_FIELD(__regoff, REG_ICP_RNF_BYTE_OFFSET_MSK, REG_ICP_RNF_BYTE_OFFSET_POS) << 2)

/** NOTE: This method is not design agnostic and only works for the 8N design */
#define CPU_TO_ICP_RNF_UNITID(__cpuuid)	(MISC0_UNITID +  ROW_CPU(__cpuuid))

/*****************************************************************************/

/** Get the SR0 Function and Unit Base Address */
#define FUNC_UNIT_SR0_BASEADDR(__fid, __uid)	(((__fid << 7) + __uid) << 12)

/**
 * Get CPU MMR Base Address in PCL Regbus Region
 */
#define CPU_MMR(__pclbase, __cpuuid, __regoff)				\
({									\
	__pclbase +							\
	FUNC_UNIT_SR0_BASEADDR(REG_CPU_FUNCID(__regoff), __cpuuid) +	\
	REG_CPU_WORD_OFFSET(__regoff);					\
})

/**
 * Get L3M MMR Base Address in PCL Regbus Region
 */
#define L3M_MMR(__pclbase, __cpuuid, __regoff)				\
({									\
	__pclbase +							\
	FUNC_UNIT_SR0_BASEADDR(REG_L3M_FUNCID(__regoff),		\
			       CPU_TO_L3M_UNITID(__cpuuid)) +		\
	REG_L3M_WORD_OFFSET(__regoff);					\
})

/**
 * Get ICC/ICP MMR Base Address in PCL Regbus Region
 */
#define ICC_ICP_MMR(__pclbase, __cpuuid, __regoff)			\
({									\
	__pclbase +							\
	FUNC_UNIT_SR0_BASEADDR(REG_ICC_ICP_CLOCKING_FUNCID(__regoff),	\
			       CPU_TO_ICC_ICP_CLOCKING_UNITID(__cpuuid)) +\
	REG_ICC_ICP_CLOCKING_WORD_OFFSET(__regoff);			\
})

#define MISC_MMR(__pclbase, __cpuuid, __regoff)				\
({									\
	__pclbase +							\
	FUNC_UNIT_SR0_BASEADDR(REG_ICP_RNF_FUNCID(__regoff),		\
			       CPU_TO_ICP_RNF_UNITID(__cpuuid)) +	\
	REG_ICP_RNF_WORD_OFFSET(__regoff);				\
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
		__rc = v2_read(cluster, addr, &__val);			\
		if (!__rc) {							\
			V2_SET_FIELD(__val, mask, pos, data);		\
			__rc = v2_write(cluster, addr, __val);		\
			if (__rc) {						\
				v2_error("reg write fail-addr:%llx cluster:%u",\
						addr, cluster);			\
			}							\
		} else	{							\
			v2_error("reg read fail-addr:%llx cluster:%u"		\
					, addr, cluster);			\
		}								\
	} while(0);								\
	__rc;									\
})

#endif /* __V2_PCL_INIT_H__ */
