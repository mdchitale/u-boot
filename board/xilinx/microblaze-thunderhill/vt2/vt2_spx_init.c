/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 */

#define v2_fmt(fmt) "SPX: " fmt

#include <vt2_spx_init.h>

static struct v2_fuses v2_fuse_config[V2_MAX_CLUSTER];

/**
 * Get the cluster fuse configs
 */
struct v2_fuses *v2_spx_get_fuse_config(vmsplat_uint32_t cluster_id)
{
	return &v2_fuse_config[cluster_id];
}

/**
 * Read the cluster configs from efuse
 */
int v2_read_fuses(vmsplat_uint32_t cluster_id, vmsplat_uint64_t ckgn_base)
{
	// TODO for VT2. Details not defined yet.
	
	return VMSPLAT_OK;
}

/**
 * Writes each 64-bit SATT register 32-bit wise.
 */
static int v2_write_satt(vmsplat_uint32_t cluster_id,
			 vmsplat_uint64_t reg_offset,
			 vmsplat_uint64_t addr)
{
	int rc;
	vmsplat_uint32_t hi, lo, rval_hi = 0, rval_lo = 0;
	vmsplat_uint64_t read_addr;
	
	/* Set PCL Match Enable bit to allow PCL accesses */
	V2_SET_FIELD(addr, VT2_SMRC_DBIDDVECSTAGING_ENABLE_MSK,
			  VT2_SMRC_DBIDDVECSTAGING_ENABLE_POS, _ULL(1));

	/* Set Load bit to allow loading to mapping register atomically */
	V2_SET_FIELD(addr, VT2_SMRC_DBIDDVECSTAGING_LOAD_MSK,
			  VT2_SMRC_DBIDDVECSTAGING_LOAD_POS, _ULL(1));

	lo = (addr & 0xffffffff);
	hi = (addr >> 32) & 0xffffffff;

	rc = v2_write(cluster_id, reg_offset, lo);
	if (rc) {
		v2_error("Failed to write to SATT\n");
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_write(cluster_id, reg_offset + 4, hi);
	if (rc) {
		v2_error("Failed to write to SATT\n");
		return VMSPLAT_ERR_EIO;
	}
	
	rc = v2_read(cluster_id, reg_offset, &rval_lo);
	if (rc) {
		v2_error("Failed to read the SATT register");
		return rc;
	}
	
	rc = v2_read(cluster_id, reg_offset + 4, &rval_hi);
	if (rc) {
		v2_error("Failed to read the SATT register");
		return rc;
	}
	
	read_addr = rval_hi;
	read_addr = read_addr << 32 | rval_lo;
	
	v2_debug("RB REG[0x%"PRIx64"], RB Addr[0x%"PRIx64"]", reg_offset, read_addr);
	
	return VMSPLAT_OK;
}

/**
 * Check Chiplet BISR to continue the chiplet initialization.
 */
int v2_check_bisr(vmsplat_uint32_t cluster_id, vmsplat_uint64_t smrc_base)
{
	int rc;
	vmsplat_uint8_t bisrdone = 0, bisrgo = 0;
	vmsplat_uint32_t val;

	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(smrc_base, VT2_SMRC_BISRCONTROLSTATUS_OFFSET),
			     VT2_SMRC_BISRCONTROLSTATUS_BISRRUN_MSK,
			     VT2_SMRC_BISRCONTROLSTATUS_BISRRUN_POS,
			     1);
	if (rc) {
		v2_error("Failed to set BisrRun in BisrControlStatus");
		return VMSPLAT_ERR_EIO;
	}

	while (!bisrdone) {
		rc = v2_read(cluster_id,
			     V2_REG_32(smrc_base, VT2_SMRC_BISRCONTROLSTATUS_OFFSET),
			     &val);
		if (rc) {
			v2_error("Failed to read BisrControlStatus");
			return VMSPLAT_ERR_EIO;
		}

		bisrdone = V2_GET_FIELD(val,
					VT2_SMRC_BISRCONTROLSTATUS_BISRDONE_MSK,
					VT2_SMRC_BISRCONTROLSTATUS_BISRDONE_POS);
	}

	rc = v2_read(cluster_id,
		     V2_REG_32(smrc_base, VT2_SMRC_BISRCONTROLSTATUS_OFFSET),
		     &val);
	if (rc) {
		v2_error("Failed to read BisrControlStatus");
		return VMSPLAT_ERR_EIO;
	}

	bisrgo = V2_GET_FIELD(val,
			      VT2_SMRC_BISRCONTROLSTATUS_BISRGO_MSK,
			      VT2_SMRC_BISRCONTROLSTATUS_BISRGO_POS);

	if (!bisrgo) {
		v2_error("Bisr failed, chiplet init terminated");
		return VMSPLAT_ERR_EFAIL;
	}

	/** Use previous BisrControlStatus read value */
	V2_SET_FIELD(val, VT2_SMRC_BISRCONTROLSTATUS_BISRRUN_MSK, VT2_SMRC_BISRCONTROLSTATUS_BISRRUN_POS, 0);
	V2_SET_FIELD(val, VT2_SMRC_BISRCONTROLSTATUS_BISRENPCL_MSK, VT2_SMRC_BISRCONTROLSTATUS_BISRENPCL_POS, 1);
	V2_SET_FIELD(val, VT2_SMRC_BISRCONTROLSTATUS_BISRENSPX_MSK, VT2_SMRC_BISRCONTROLSTATUS_BISRENSPX_POS, 0);
	
	rc = v2_write(cluster_id, V2_REG_32(smrc_base, VT2_SMRC_BISRCONTROLSTATUS_OFFSET), val);
	if (rc) {
		v2_error("failed to write BisrControlStatus");
		return VMSPLAT_ERR_EFAIL;
	}
	
	return VMSPLAT_OK;
}

int v2_config_satt(vmsplat_uint32_t cluster_id, vmsplat_uint64_t smrc_base,
		   const struct v2_mmio_bases *v2_mmio)
{
	int rc;
	vmsplat_uint64_t regaddr, val;

	/* DVEC Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_DBIDDVECSTAGING_OFFSET);
	val = v2_mmio->dbid_dvec_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure DVec");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("DVecBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* DReg Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_DBIDDREGSTAGING_OFFSET);
	val = v2_mmio->dbid_dreg_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure DReg");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("DRegBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* DPuC Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_DBIDPUCSTAGING_OFFSET);
	val = v2_mmio->dbid_puc_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure Debug PuC Space");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("DPucBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* LWIC Mtimer Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_LWICMMODETIMERSTAGING_OFFSET);
	val = v2_mmio->lwic_mtime_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure LWIC");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("LwicMMode [0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* SMRC Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_SMRCSTAGING_OFFSET);
	val = v2_mmio->smrc_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure SMRC");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("SmrcBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* CHIB Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_CHIBSTAGING_OFFSET);
	val = v2_mmio->chib_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure CHIB");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("ChibBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* SDDI Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_CHPLSTAGING_OFFSET);
	val = v2_mmio->chpl_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure CHPL");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("SddiBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* CKGN Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_CKGNSTAGING_OFFSET);
	val = v2_mmio->ckgn_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure CKGN");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("CkgnBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/* I3C APB Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_I3CSTAGING_OFFSET);
	val = v2_mmio->i3c_apb_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure I3C_APB");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("I3CBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	/*  PCL Regbus Base */
	regaddr = V2_REG_64(smrc_base, VT2_SMRC_PCLSTAGING_OFFSET);
	val = v2_mmio->pcl_base;
	rc = v2_write_satt(cluster_id, regaddr, val);
	if (rc) {
		v2_error("Failed to configure PCL regbus");
		return VMSPLAT_ERR_EIO;
	}
	v2_debug("PCLBase[0x%"PRIx64"],ADDR[0x%"PRIx64"]", regaddr, val);

	return VMSPLAT_OK;
}

/**
 * Enable MTIMER
 *
 * This function must be called after the v2_system_deassert
 * and before the v2_cluster_deassert
 */
int v2_enable_mtime(vmsplat_uint32_t cluster_id, vmsplat_uint64_t lwic_base)
{
	int rc;
	vmsplat_uint32_t en_val = 0;
	vmsplat_uint32_t div_val = 0;
	vmsplat_uint32_t readval = 0;
	vmsplat_uint64_t regaddr_timerincen, regaddr_timerdiv;

	regaddr_timerincen = V2_REG_64(lwic_base, VT2_LWIC_TIMERINCEN_OFFSET);
	regaddr_timerdiv = V2_REG_64(lwic_base, VT2_LWIC_TIMERINCDIV_OFFSET);

	rc = v2_read(cluster_id, regaddr_timerincen, &en_val);
	if (rc) {
		v2_error("Failed to read MTIME ENABLE");
		return VMSPLAT_ERR_EIO;
	}

	/* DISABLE MTIME before changing DIV */
	if (V2_GET_FIELD(en_val, VT2_LWIC_TIMERINCEN_MSK, VT2_LWIC_TIMERINCEN_POS)) {
		V2_SET_FIELD(en_val, VT2_LWIC_TIMERINCEN_MSK,
			     VT2_LWIC_TIMERINCEN_POS,
			     0);

		rc = v2_write(cluster_id, regaddr_timerincen, en_val);
		if (rc) {
			v2_error("Failed to write to MTIME ENABLE");
			return VMSPLAT_ERR_EIO;
		}
	}

	/* Update MTIME DIV(0b00) value for 100MHz MTIME Increment Frequency
	 * REFCLK -> 100MHz */
	V2_SET_FIELD(div_val, VT2_LWIC_TIMERINCDIV_MSK,
		     VT2_LWIC_TIMERINCDIV_POS, V2_MTIMER_DIV_0);
	
	rc = v2_write(cluster_id, regaddr_timerdiv, div_val);
	if (rc) {
		v2_error("Failed to write to TimerIncDiv 32-Bit Low");
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_write(cluster_id, (regaddr_timerdiv + 4), 0);
	if (rc) {
		v2_error("Failed to write to TimerIncDiv 32-Bit Upper");
		return VMSPLAT_ERR_EIO;
	}

	/* Need to wait after updating DIV */
	v2_nsdelay(V2_MTIME_DIV_DELAY); /* 8 REFCLK cycles plus 8 SCLK cycles */
	
	/* Enable MTIME */
	V2_SET_FIELD(en_val, VT2_LWIC_TIMERINCEN_ENABLE_MSK,
		     VT2_LWIC_TIMERINCEN_ENABLE_POS, 1);

	rc = v2_write(cluster_id, (regaddr_timerincen + 4), 0);
	if (rc) {
		v2_error("Failed to write to TimerIncEn 32-Bit Upper");
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_write(cluster_id, regaddr_timerincen, en_val);
	if (rc) {
		v2_error("Failed to write to TimerIncEn 32-Bit Low");
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_read(cluster_id, regaddr_timerincen, &readval);
	if (rc) {
		v2_error("Failed to read MTIME ENABLE");
		return VMSPLAT_ERR_EIO;
	}

	v2_debug("TimerIncEn[0x%"PRIx64"], Write: %x, ReadBack: %x",
		  regaddr_timerincen,
		  en_val, readval);

	return VMSPLAT_OK;
}

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
		       vmsplat_uint16_t cluster_node_id1)
{
	int rc;
	vmsplat_uint32_t cluster_impid;
	vmsplat_uint32_t val = 0, rnf_vec0, rnf_vec1;

	V2_SET_FIELD(val, VT2_SMRC_CLUSTERCONFIG_CPUVECTOR_MSK,
		     VT2_SMRC_CLUSTERCONFIG_CPUVECTOR_POS,
		     cpu_vector);

	/**
	 * Specifies the number of L3M slices that are enabled, using an N-1 encoding:
	 * 0x00 - Reserved (1 slice)
	 * 0x01 - Reserved (2 slices)
	 * 0x03 - 4 slices
	 * 0x07 - 8 slices
	 * 0x0F - 16 slices
	 */
	V2_SET_FIELD(val, VT2_SMRC_CLUSTERCONFIG_NUMBERL3MSLICE_MSK,
		     VT2_SMRC_CLUSTERCONFIG_NUMBERL3MSLICE_POS,
		     num_l3m_slices - 1);

	/**
	 * Specifies the capacity of each L3M slice:
	 * 0x2 - 1024 sets (1MB, 10b index)
	 * 0x3 - 2048 sets (2MB, 11b index)
	 * 0x4 - 4096 sets (4MB, 12b index)
	 */
	V2_SET_FIELD(val, VT2_SMRC_CLUSTERCONFIG_L3MSLICECAP_MSK,
		     VT2_SMRC_CLUSTERCONFIG_L3MSLICECAP_POS,
		     l3m_cap);

	rc = v2_write(cluster_id, V2_REG_32(smrc_base, VT2_SMRC_CLUSTERCONFIG_OFFSET), val);
	if (rc) {
		v2_debug("Failed to update cluster config register");
		return VMSPLAT_ERR_EIO;
	}

	/**
	 * To enable all four RN-F interfaces, the computation
	 * RnfVec1[47:6] ^ RnfVec0[47:6] must not equal zero.
	 *
	 * To enable only two RN-F interfaces,
	 * RnfVec1[47:6] must equal zero, and RnfVec0[47:6] must not equal zero.
	 *
	 * To enable only one RN-F interface,
	 * RnfVec1[47:6] and RnfVec0[47:6] must equal zero.
	 *
	 * Note: RN-F interface 0b00 must always be enabled for UC accesses.
	 */

	/**
	 * Get the RN-F count using ClusterImpId
	 * Read ICPCONFIG register of ICP0 since atleast one RN-F interface
	 * has to be present in the cluster.
	 */
	rc = v2_read(cluster_id,
		     MISC_MMR(pcrb_base, 0, VT2_ICP_ICPCONFIG_OFFSET),
		     &val);
	if (rc) {
		v2_error("failed to read IcpConfig");
		return VMSPLAT_ERR_EIO;
	}

	cluster_impid = V2_GET_FIELD(val, VT2_ICP_ICPCONFIG_CLUSTERIMPID_MSK,
				      VT2_ICP_ICPCONFIG_CLUSTERIMPID_POS);

	if (cluster_impid == V2_CLUSTER_IMPID_1) {
		/**
		 * 2 RN-F of 4 RN-F interfaces.
		 *
		 * For 2 RN-F interface:
		 * RnfVec1[47:6] must equal zero, and RnfVec0[47:6] must
		 * not equal zero
		 *
		 * For 4 RN-F interface:
		 * The computation RnfVec1[47:6] ^ RnfVec0[47:6] must
		 * not equal zero
		 */
		rnf_vec1 = 0x0;
		rnf_vec0 = 0xffff;
	}
	else {
		/**
		 * 1 RN-F interface
		 * RnfVec1[47:6] and RnfVec0[47:6] must equal zero
		 **/
		rnf_vec1 = 0x0;
		rnf_vec0 = 0x0;
	}

	/** Program the RN-F Vector0 */
	val = 0;
	V2_SET_FIELD(val, VT2_SMRC_RNFSELVEC0LO_RNFVECLO_MSK,
		     VT2_SMRC_RNFSELVEC0LO_RNFVECLO_POS,
		     rnf_vec0);

	rc = v2_write(cluster_id, V2_REG_32(smrc_base, VT2_SMRC_RNFSELVEC0LO_OFFSET), val);
	if (rc) {
		v2_debug("Failed to write RnfSelVec0Lo: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	val = 0;
	V2_SET_FIELD(val, VT2_SMRC_RNFSELVEC0HI_RNFVECHI_MSK,
		     VT2_SMRC_RNFSELVEC0HI_RNFVECHI_POS,
		     rnf_vec0);

	rc = v2_write(cluster_id, V2_REG_32(smrc_base, VT2_SMRC_RNFSELVEC0HI_OFFSET), val);
	if (rc) {
		v2_debug("Failed to write RnfSelVec0Hi: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	/** Program the RN-F Vector1 */
	val = 0;
	V2_SET_FIELD(val, VT2_SMRC_RNFSELVEC1LO_RNFVECLO_MSK,
			VT2_SMRC_RNFSELVEC1LO_RNFVECLO_POS,
			rnf_vec1);

	rc = v2_write(cluster_id, V2_REG_32(smrc_base, VT2_SMRC_RNFSELVEC1LO_OFFSET), val);
	if (rc) {
		v2_debug("Failed to write RnfSelVec1Lo: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}
	
	val = 0;
	V2_SET_FIELD(val, VT2_SMRC_RNFSELVEC1HI_RNFVECHI_MSK,
			VT2_SMRC_RNFSELVEC1HI_RNFVECHI_POS,
			rnf_vec1);

	rc = v2_write(cluster_id, V2_REG_32(smrc_base, VT2_SMRC_RNFSELVEC1HI_OFFSET), val);
	if (rc) {
		v2_debug("Failed to write RnfSelVec1Hi: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	/**
	 * Program the ChibConfigHigh with ClusterNodeID and ClusterNS
	 *
	 * - The ClusterNodeID must be unique to both CHIB0 and CHIB1
	 * and all other node IDs across the system.
	 * - The ClusterNS bit must be set to the same value in both
	 * ChiConfighHigh registers.
	 */
	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(chib_base, VT2_CHIB_CHICONFIGHIGH_OFFSET),
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNODEID_MSK,
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNODEID_POS,
			     cluster_node_id0);
	if (rc) {
		v2_debug("Failed to write ClusterNodeID in CHIB0: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(chib_base + V2_CHIB_CHIB1_OFFSET, VT2_CHIB_CHICONFIGHIGH_OFFSET),
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNODEID_MSK,
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNODEID_POS,
			     cluster_node_id1);
	if (rc) {
		v2_debug("Failed to write ClusterNodeID in CHIB1: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(chib_base, VT2_CHIB_CHICONFIGHIGH_OFFSET),
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNS_MSK,
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNS_POS,
			     cluster_ns);
	if (rc) {
		v2_debug("Failed to write ClusterNS in CHIB0: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(chib_base + V2_CHIB_CHIB1_OFFSET, VT2_CHIB_CHICONFIGHIGH_OFFSET),
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNS_MSK,
			     VT2_CHIB_CHICONFIGHIGH_CLUSTERNS_POS,
			     cluster_ns);
	if (rc) {
		v2_debug("Failed to write ClusterNS in CHIB1: %d\n", rc);
		return VMSPLAT_ERR_EIO;
	}

	/* Step 2: Deassert NonCoreHardReset in NonCoreRstCtl */
	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(smrc_base, VT2_SMRC_CLUSTERRESET_OFFSET),
			     VT2_SMRC_CLUSTERRESET_NONCORERESET_MSK,
			     VT2_SMRC_CLUSTERRESET_NONCORERESET_POS,
			     0);
	if (rc) {
		v2_debug("Failed to deassert NonCoreHardReset");
		return rc;
	}
	/**
	 * NOTE: mtimer must initialized between NonCoreReset and enabling of
	 * PCL Regbus.
	 */
	rc = v2_enable_mtime(cluster_id, lwic_base);
	if (rc) {
		v2_error("failed to enable mtime");
		return VMSPLAT_ERR_EFAIL;
	}

	/* Step 3: Enable PclRspBridgeEnable */
	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(smrc_base, VT2_SMRC_CLUSTERRESET_OFFSET),
			     VT2_SMRC_CLUSTERRESET_PCLRSPBRIDGEENABLE_MSK,
			     VT2_SMRC_CLUSTERRESET_PCLRSPBRIDGEENABLE_POS,
			     1);
	if (rc) {
		v2_error("Failed to set PclRspBridgeEnable");
		return rc;
	}

	return VMSPLAT_OK;
}

int v2_dbmd_harts_enable(vmsplat_uint32_t cluster_id, vmsplat_uint32_t dbmd_base)
{
	int rc = 0;
	vmsplat_uint32_t cpu = 0, rval = 0;
	
	v2_debug("V2 DBMD Enabling Harts in HartAvailableStatus");
	rc = v2_write(cluster_id,
		       V2_REG_32(dbmd_base, VT2_DBMD_HARTSELECT_OFFSET),
		       cpu);
	if (rc) {
		v2_error("Write DBMD HartSelect Register: Hart-%u %d", cpu, rc);
		return rc;
	}

	rc = v2_write(cluster_id,
		       V2_REG_32(dbmd_base, VT2_DBMD_HARTAVAILABLESTATUS_OFFSET),
		       1);
	if (rc) {
		v2_error("Write DBMD HartAvailableStatus Register: Hart-%u %d", cpu, rc);
		return rc;
	}

	/*vt1_debug("Selecting Hart-%u by writing %u in HartSelect[%p]", cpu, cpu,
		  (void *)VT1_REG_32(dbmd_base, VT2_DBMD_HARTSELECT_OFFSET));

	vt1_debug("Enable Hart-%u in HartAvailableStatus[%p]", cpu,
		  (void *)VT1_REG_32(dbmd_base, VT2_DBMD_HARTAVAILABLESTATUS_OFFSET)); */

	rc = v2_read(cluster_id,
		       V2_REG_32(dbmd_base, VT2_DBMD_HARTSELECT_OFFSET),
		       &rval);
	if (rc) {
		v2_error("Write DBMD HartSelect Register: Hart-%u %d", cpu, rc);
		return rc;
	}
	
	v2_debug("Read HardSelect[0x%"PRIx64"]: ReadBack Value: %x",
		(vmsplat_uint64_t)V2_REG_32(dbmd_base, VT2_DBMD_HARTSELECT_OFFSET), rval);

	rval = 0;
	rc = v2_read(cluster_id,
		     V2_REG_32(dbmd_base, VT2_DBMD_HARTAVAILABLESTATUS_OFFSET),
		     &rval);
	if (rc) {
		v2_error("Write DBMD HartAvailableStatus Register: Hart-%u %d", cpu, rc);
		return rc;
	}

	v2_debug("Read HardAvailableStatus[0x%"PRIx64"]: ReadBack Value: %x",
	(vmsplat_uint64_t)V2_REG_32(dbmd_base, VT2_DBMD_HARTAVAILABLESTATUS_OFFSET), rval);

	return rc;
}

int v2_dbmd_enable(vmsplat_uint32_t cluster_id, vmsplat_uint64_t dbmd_base)
{
	int rc = 0;
	vmsplat_uint32_t val = 0, dmactive = 0;
	
	rc = v2_read(cluster_id,
		     V2_REG_32(dbmd_base, VT2_DBMD_DMCONTROL_OFFSET),
		     &val);
	if (rc) {
		v2_error("Read DBMD DmControl Register: %d", rc);
		return rc;
	}
	
	/* Read the DmControl.DmActive bit */
	dmactive = V2_GET_FIELD(val,
				 VT2_DBMD_DMCONTROL_DMACTIVE_MSK,
				 VT2_DBMD_DMCONTROL_DMACTIVE_POS);
	/* Read on DmControl.DmActive == 1 means Debug Module is enabled */
	if (dmactive) {
		v2_info("Debug Module already enabled");
		return VMSPLAT_ERR_EALREADY;
	}
	
	/* Write DmControl.DmActive = 1 to start the DM activation sequence */
	rc = CLUSTER_REG_RMW(cluster_id,
			     V2_REG_32(dbmd_base, VT2_DBMD_DMCONTROL_OFFSET),
			     VT2_DBMD_DMCONTROL_DMACTIVE_MSK,
			     VT2_DBMD_DMCONTROL_DMACTIVE_POS,
			     1);
	if (rc) {
		v2_error("Write DBMD DmControl.DmActive: %d", rc);
		return rc;
	}
	
	val = 0;
	/* DmControl.DmActive = 1 drives the DmActiveControlStatus.DmActiveRequestedState == 1 */
	/* Poll for DmActiveControlStatus.DmactiveRequestedState == 1 */
	while (!dmactive) {
		rc = v2_read(cluster_id,
			     V2_REG_32(dbmd_base, VT2_DBMD_DMACTIVECONTROLSTATUS_OFFSET),
			     &val);
		if (rc) {
			v2_error("Read DBMD DmActiveControlStatus Register: %d", rc);
			return rc;
		}

		dmactive = V2_GET_FIELD(val,
					 VT2_DBMD_DMACTIVECONTROLSTATUS_DMACTIVEREQUESTEDSTATE_MSK,
					 VT2_DBMD_DMACTIVECONTROLSTATUS_DMACTIVEREQUESTEDSTATE_POS);
	}

	/* Checkpoint: DmControl.DmActive == 1,
	 * DmActiveControlStatus.DmActiveRequestedState == 1
	 *
	 * Write the DmActiveControlStatus.DmActiveCurrentState = 1 */
	val = 0;
	V2_SET_FIELD(val, VT2_DBMD_DMACTIVECONTROLSTATUS_DMACTIVECURRENTSTATE_MSK,
		      VT2_DBMD_DMACTIVECONTROLSTATUS_DMACTIVECURRENTSTATE_POS,
		      1);
	
	rc = v2_write(cluster_id,
		       V2_REG_32(dbmd_base, VT2_DBMD_DMACTIVECONTROLSTATUS_OFFSET),
		       val);
	if (rc) {
		v2_error("Failed to write DbmdActiveControlStatus");
		return VMSPLAT_ERR_EIO;
	}
	
	/* 20 sec delay to let the DM status gets reflected into
	 * DmControl.DmActive. The delay is arbitrary and can be
	 * fine tuned if required */
	v2_nsdelay(20);
	
	val = 0;
	rc = v2_read(cluster_id,
		      V2_REG_32(dbmd_base, VT2_DBMD_DMACTIVECONTROLSTATUS_OFFSET),
		      &val);
	if (rc) {
		v2_error("Read DBMD DmActiveControlStatus Register: %d", rc);
		return rc;
	}

	v2_debug("DmActiveControlStatus[0x%"PRIx64"] Register Read Value: %x",
		(vmsplat_uint64_t)V2_REG_32(dbmd_base, VT2_DBMD_DMACTIVECONTROLSTATUS_OFFSET), val);
	
	val = 0;
	rc = v2_read(cluster_id,
		      V2_REG_32(dbmd_base, VT2_DBMD_DMCONTROL_OFFSET),
		      &val);
	if (rc) {
		v2_error("Read DBMD DmControl Register: %d", rc);
		return rc;
	}

	v2_debug("DmControl[0x%"PRIx64"] Register Read Value: %x",
		(vmsplat_uint64_t)V2_REG_32(dbmd_base, VT2_DBMD_DMCONTROL_OFFSET), val);

	/* Checkpoint: DM is enabled. Enable harts visibility to debugger */
	rc = v2_dbmd_harts_enable(cluster_id, dbmd_base);
	if (rc) {
		v2_error("Failed to enable harts in HartAvailableStatus - %d", rc);
	}

	return rc;
}
