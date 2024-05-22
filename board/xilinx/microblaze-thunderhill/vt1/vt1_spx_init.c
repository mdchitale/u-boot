/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#define vt1_fmt(fmt) "SPX: " fmt
#define DEBUG
#include <vt1/hw/vt1_efuse_mmr.h>
#include <vt1/vt1_spx_init.h>

/*
 *  allocating for max possible, revisit to optimize
 */
static struct vt1_fuses efuse[VT1_MAX_CLUSTER];

struct vt1_fuses *vt1_spx_get_efuse(vmsplat_uint32_t apcluster_idx)
{
	return &efuse[apcluster_idx];
}

int vt1_read_fuses(vmsplat_uint32_t apcluster_idx,
		   vmsplat_uint64_t ckgn_base,
		   vmsplat_uint32_t num_harts)
{
	int rc = 0, i;
	vmsplat_uint32_t fuse_array[32];

	/* Set CTL - Read flops array */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     VT1_REG_32(ckgn_base, VT1_CKGN_EFUSECTL_OFFSET),
			     VT1_CKGN_EFUSECTL_RDWRCTL_MSK,
			     VT1_CKGN_EFUSECTL_RDWRCTL_POS,
			     0x2);
	if (rc)
		return rc;

	/* Read all 32 values */
	for (i = 0; i < 32; i++) {
		rc = vt1_read(apcluster_idx,
				  VT1_REG_32(ckgn_base, VT1_CKGN_EFUSEATADDR_OFFSET(i)),
				  &fuse_array[i]);
		if (rc)
			break;
	}

	/* No Read/Write */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     VT1_REG_32(ckgn_base, VT1_CKGN_EFUSECTL_OFFSET),
			     VT1_CKGN_EFUSECTL_RDWRCTL_MSK,
			     VT1_CKGN_EFUSECTL_RDWRCTL_POS,
			     0x0);
	if (rc)
		return rc;

	/* TODO - The contents of eFuse are no defined at. */
	/* Setting to default values(5nm) */

	if (num_harts == 8) {
		efuse[apcluster_idx].cpu_vector = 0xFF;
		efuse[apcluster_idx].num_ul3_slices = 7; /* N-1 */
		efuse[apcluster_idx].ul3_cap = 0x4;
		efuse[apcluster_idx].bclk_base_div = 12;
		efuse[apcluster_idx].bclk_max_div = 4095;
		efuse[apcluster_idx].nclk_base_div = 12;
		efuse[apcluster_idx].nclk_max_div = 4095;
		efuse[apcluster_idx].cclk_base_div = 12;
		efuse[apcluster_idx].cclk_max_div = 4095;
	} else if (num_harts == 16) {
		efuse[apcluster_idx].cpu_vector = 0xFFFF;
		efuse[apcluster_idx].num_ul3_slices = 15; /* N-1 */
		efuse[apcluster_idx].ul3_cap = 0x4;
		efuse[apcluster_idx].bclk_base_div = 12;
		efuse[apcluster_idx].bclk_max_div = 4095;
		efuse[apcluster_idx].nclk_base_div = 12;
		efuse[apcluster_idx].nclk_max_div = 4095;
		efuse[apcluster_idx].cclk_base_div = 12;
		efuse[apcluster_idx].cclk_max_div = 4095;
	}

	return rc;
}

static int vt1_write_satt_32(vmsplat_uint32_t apcluster_idx,
			     vmsplat_uint64_t reg_offset,
			     vmsplat_uint64_t addr)
{
	int rc;
	vmsplat_uint32_t hi, lo, rval_hi = 0, rval_lo = 0;
	vmsplat_uint64_t read_addr;
	
	/* Set bit 63 and 62 */
	VT1_SET_FIELD(addr, VT1_SMRC_DBIDDVECSTAGING_ENABLE_MSK,
			  VT1_SMRC_DBIDDVECSTAGING_ENABLE_POS, _ULL(1));
	VT1_SET_FIELD(addr, VT1_SMRC_DBIDDVECSTAGING_LOAD_MSK,
			  VT1_SMRC_DBIDDVECSTAGING_LOAD_POS, _ULL(1));

	lo = (addr & 0xffffffff);
	hi = (addr >> 32) & 0xffffffff;

	rc = vt1_write(apcluster_idx, reg_offset, lo);
	if (rc) {
		vt1_error("Failed to write to SATT\n");
		return VMSPLAT_ERR_EIO;
	}

	rc = vt1_write(apcluster_idx, reg_offset + 4, hi);
	if (rc) {
		vt1_error("Failed to write to SATT\n");
		return VMSPLAT_ERR_EIO;
	}
	
	rc = vt1_read(apcluster_idx, reg_offset, &rval_lo);
	if (rc) {
		vt1_error("Failed to read the SATT register");
		return rc;
	}
	
	rc = vt1_read(apcluster_idx, reg_offset + 4, &rval_hi);
	if (rc) {
		vt1_error("Failed to read the SATT register");
		return rc;
	}
	
	read_addr = rval_hi;
	read_addr = read_addr << 32 | rval_lo;
	
	vt1_debug("RB REG[%p], RB Addr[%p]", (void *)reg_offset, (void *)read_addr);
	
	return VMSPLAT_OK;
}

int vt1_config_satt(vmsplat_uint32_t apcluster_idx,
		    vmsplat_uint64_t smrc_base,
		    const struct vt1_mmio_bases *vt1_mmio)
{
	int rc;
	vmsplat_uint64_t addr;

	/* DVEC Base */
	addr = vt1_mmio->DBID_DVec_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_DBIDDVECSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure DVec");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("DVecBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_DBIDDVECSTAGING_OFFSET),
		  (void *)addr);

	/* DReg Base */
	addr = vt1_mmio->DBID_DReg_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_DBIDDREGSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure DReg");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("DRegBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_DBIDDREGSTAGING_OFFSET),
		  (void *)addr);


	/* DPuC Base */
	addr = vt1_mmio->DBID_PuC_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_DBIDPUCSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure Debug PuC Space");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("DPucBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_DBIDPUCSTAGING_OFFSET),
		  (void *)addr);

	/* LWIC Mtimer Base */
	addr = vt1_mmio->LWIC_Mtime_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_LWICMMODETIMERSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure LWIC");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("LwicMMode [%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_LWICMMODETIMERSTAGING_OFFSET),
		  (void *)addr);

	/* SMRC Base */
	addr = vt1_mmio->SMRC_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_SMRCSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure SMRC");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("SmrcBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_SMRCSTAGING_OFFSET),
		  (void *)addr);

	/* CHIB Base */
	addr = vt1_mmio->CHIB_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_CHIBSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure CHIB");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("ChibBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_CHIBSTAGING_OFFSET),
		  (void *)addr);

	/* SDDI Base */
	addr = vt1_mmio->SDDI_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_SDDISTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure SDDI");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("SddiBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_SDDISTAGING_OFFSET),
		  (void *)addr);

	/* CKGN Base */
	addr = vt1_mmio->CKGN_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_CKGNSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure CKGN");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("CkgnBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_CKGNSTAGING_OFFSET),
		  (void *)addr);

	/* I3C APB Base */
	addr = vt1_mmio->I3C_APB_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_I3CSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure I3C_APB");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("I3CBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_I3CSTAGING_OFFSET),
		  (void *)addr);

	/*  PCL Regbus Base */
	addr = vt1_mmio->PCL_regbus_base;
	rc = vt1_write_satt_32(apcluster_idx,
			       VT1_REG_64(smrc_base, VT1_SMRC_PCLSTAGING_OFFSET),
			       addr);
	if (rc) {
		vt1_error("Failed to configure PCL regbus");
		return VMSPLAT_ERR_EIO;
	}
	vt1_debug("PCLBase[%p],ADDR[%p]",
		  (void *)VT1_REG_64(smrc_base, VT1_SMRC_PCLSTAGING_OFFSET),
		  (void *)addr);

	return VMSPLAT_OK;
}

int vt1_enable_mtime(vmsplat_uint32_t apcluster_idx,
		     vmsplat_uint64_t lwic_base)
{
	int rc;
	vmsplat_uint32_t en_val = 0;
	vmsplat_uint32_t div_val = 0;
	vmsplat_uint32_t readval = 0;

	rc = vt1_read(apcluster_idx,
			  VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCEN_OFFSET),
			  &en_val);
	if (rc) {
		vt1_error("Failed to read MTIME ENABLE");
		return VMSPLAT_ERR_EIO;
	}

	/* DISABLE MTIME before changing DIV */
	if (VT1_GET_FIELD(en_val, VT1_LWIC_TIMERINCEN_MSK, VT1_LWIC_TIMERINCEN_POS)) {
		VT1_SET_FIELD(en_val, VT1_LWIC_TIMERINCEN_MSK,
				  VT1_LWIC_TIMERINCEN_POS,
				  0);

		rc = vt1_write(apcluster_idx,
				   VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCEN_OFFSET),
				   en_val);
		if (rc) {
			vt1_error("Failed to write to MTIME ENABLE");
			return VMSPLAT_ERR_EIO;
		}
	}

	/* Update MTIME DIV(0b00) value for 100MHz MTIME Increment Frequency
	 * REFCLK -> 100MHz */
	VT1_SET_FIELD(div_val, VT1_LWIC_TIMERINCDIV_MSK,
			  VT1_LWIC_TIMERINCDIV_POS,
			  VT1_MTIMER_DIV_0);
	
	rc = vt1_write(apcluster_idx,
		       VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCDIV_OFFSET),
		       div_val);
	if (rc) {
		vt1_error("Failed to write to TimerIncDiv 32-Bit Low");
		return VMSPLAT_ERR_EIO;
	}

	rc = vt1_write(apcluster_idx,
		       (VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCDIV_OFFSET) + 4),
		       0);
	if (rc) {
		vt1_error("Failed to write to TimerIncDiv 32-Bit Upper");
		return VMSPLAT_ERR_EIO;
	}

	/* Need to wait after updating DIV */
	vt1_nsdelay(VT1_MTIME_DIV_DELAY); /* 8 REFCLK cycles plus 8 SCLK cycles */
	
	/* Enable MTIME */
	VT1_SET_FIELD(en_val, VT1_LWIC_TIMERINCEN_ENABLE_MSK,
			  VT1_LWIC_TIMERINCEN_ENABLE_POS,
			  1);

	rc = vt1_write(apcluster_idx,
		       (VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCEN_OFFSET) + 4),
		       0);
	if (rc) {
		vt1_error("Failed to write to TimerIncEn 32-Bit Upper");
		return VMSPLAT_ERR_EIO;
	}

	rc = vt1_write(apcluster_idx,
		       VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCEN_OFFSET),
		       en_val);
	if (rc) {
		vt1_error("Failed to write to TimerIncEn 32-Bit Low");
		return VMSPLAT_ERR_EIO;
	}

	rc = vt1_read(apcluster_idx,
			  VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCEN_OFFSET),
			  &readval);
	if (rc) {
		vt1_error("Failed to read MTIME ENABLE");
		return VMSPLAT_ERR_EIO;
	}

	vt1_debug("TimerIncEn[%p], Write: %x, ReadBack: %x",
		  (void *)VT1_REG_64(lwic_base, VT1_LWIC_TIMERINCEN_OFFSET),
		  en_val, readval);

	return VMSPLAT_OK;
}

int vt1_system_deassert(vmsplat_uint32_t apcluster_idx,
			vmsplat_uint64_t smrc_base,
			vmsplat_uint32_t base_hartid,
			vmsplat_uint16_t cpu_vector,
			vmsplat_uint8_t num_ul3_slices,
			vmsplat_uint8_t ul3_cap)
{
	int rc;
	vmsplat_uint32_t val = 0, clusterid;

	/* Step 1: Update PCL CONFIG register */
	clusterid = VT1_GET_FIELD(base_hartid,
				  VT1_CLUSTERID_MSK,
				  VT1_CLUSTERID_POS);

	VT1_SET_FIELD(val, VT1_SMRC_CLUSTERCONFIG_CLUSTERID_MSK,
			  VT1_SMRC_CLUSTERCONFIG_CLUSTERID_POS,
			  clusterid);

	VT1_SET_FIELD(val, VT1_SMRC_CLUSTERCONFIG_CPUVECTOR_MSK,
			  VT1_SMRC_CLUSTERCONFIG_CPUVECTOR_POS,
			  cpu_vector);

	VT1_SET_FIELD(val, VT1_SMRC_CLUSTERCONFIG_NUMBERL3MSLICE_MSK,
			  VT1_SMRC_CLUSTERCONFIG_NUMBERL3MSLICE_POS,
			  num_ul3_slices);

	VT1_SET_FIELD(val, VT1_SMRC_CLUSTERCONFIG_L3MSLICECAP_MSK,
			  VT1_SMRC_CLUSTERCONFIG_L3MSLICECAP_POS,
			  ul3_cap);

	rc = vt1_write(apcluster_idx,
		       VT1_REG_32(smrc_base, VT1_SMRC_CLUSTERCONFIG_OFFSET),
		       val);
	if (rc) {
		vt1_debug("Failed to update cluster config register");
		return VMSPLAT_ERR_EIO;
	}

	/* Step 2: Deassert isolation cells from PCL to SPX */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			    VT1_REG_32(smrc_base, VT1_SMRC_CLUSTERISOLATION_OFFSET),
			    VT1_SMRC_CLUSTERISOLATION_ENABLE_MSK,
			    VT1_SMRC_CLUSTERISOLATION_ENABLE_POS,
			    0);
	if (rc) {
		vt1_debug("Failed to update cluster isolation");
		return rc;
	}

	/* Step 3: Deassert NonCoreHardReset in NonCoreRstCtl */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			    VT1_REG_32(smrc_base, VT1_SMRC_CLUSTERRESET_OFFSET),
			    VT1_SMRC_CLUSTERRESET_NONCORERESET_MSK,
			    VT1_SMRC_CLUSTERRESET_NONCORERESET_POS,
			    0);
	if (rc) {
		vt1_debug("Failed to deassert NonCoreHardReset");
		return rc;
	}

	/* Step 4: Enable PclRspBridgeEnable */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     VT1_REG_32(smrc_base, VT1_SMRC_CLUSTERRESET_OFFSET),
			     VT1_SMRC_CLUSTERRESET_PCLRSPBRIDGEENABLE_MSK,
			     VT1_SMRC_CLUSTERRESET_PCLRSPBRIDGEENABLE_POS,
			     1);
	if (rc) {
		vt1_error("Failed to set PclRspBridgeEnable");
		return rc;
	}
	
	return VMSPLAT_OK;
}

int vt1_dbmd_harts_enable(vmsplat_uint32_t apcluster_idx,
			  vmsplat_uint32_t dbmd_base,
			  vmsplat_uint32_t num_harts)
{
	int rc = 0;
	vmsplat_uint32_t cpu, rval = 0;
	
	vt1_debug("VT1 DBMD Enabling Harts in HartAvailableStatus");
	for (cpu = 0; cpu < num_harts; cpu++) {
		rc = vt1_write(apcluster_idx,
			       VT1_REG_32(dbmd_base, VT1_DBMD_HARTSELECT_OFFSET),
			       cpu);
		if (rc) {
			vt1_error("Write DBMD HartSelect Register: Hart-%u %d", cpu, rc);
			return rc;
		}

		rc = vt1_write(apcluster_idx,
			       VT1_REG_32(dbmd_base, VT1_DBMD_HARTAVAILABLESTATUS_OFFSET),
			       1);
		if (rc) {
			vt1_error("Write DBMD HartAvailableStatus Register: Hart-%u %d", cpu, rc);
			return rc;
		}

		vt1_debug("Selecting Hart-%u by writing %u in HartSelect[%p]", cpu, cpu,
			  (void *)VT1_REG_32(dbmd_base, VT1_DBMD_HARTSELECT_OFFSET));

		vt1_debug("Enable Hart-%u in HartAvailableStatus[%p]", cpu,
			  (void *)VT1_REG_32(dbmd_base, VT1_DBMD_HARTAVAILABLESTATUS_OFFSET));

		rc = vt1_read(apcluster_idx,
			       VT1_REG_32(dbmd_base, VT1_DBMD_HARTSELECT_OFFSET),
			       &rval);
		if (rc) {
			vt1_error("Write DBMD HartSelect Register: Hart-%u %d", cpu, rc);
			return rc;
		}
		vt1_debug("Read HardSelect[%p]: ReadBack Value: %x",
		(void *)VT1_REG_32(dbmd_base, VT1_DBMD_HARTSELECT_OFFSET), rval);

		rval = 0;
		rc = vt1_read(apcluster_idx,
			       VT1_REG_32(dbmd_base, VT1_DBMD_HARTAVAILABLESTATUS_OFFSET),
			       &rval);
		if (rc) {
			vt1_error("Write DBMD HartAvailableStatus Register: Hart-%u %d", cpu, rc);
			return rc;
		}

		vt1_debug("Read HardAvailableStatus[%p]: ReadBack Value: %x",
		(void *)VT1_REG_32(dbmd_base, VT1_DBMD_HARTAVAILABLESTATUS_OFFSET), rval);
	}

	return rc;
}

int vt1_dbmd_enable(vmsplat_uint32_t apcluster_idx,
		    vmsplat_uint64_t dbmd_base,
		    vmsplat_uint32_t num_harts)
{
	int rc = 0;
	vmsplat_uint32_t val = 0, dmactive = 0;
	
	rc = vt1_read(apcluster_idx,
		      VT1_REG_32(dbmd_base, VT1_DBMD_DMCONTROL_OFFSET),
		      &val);
	if (rc) {
		vt1_error("Read DBMD DmControl Register: %d", rc);
		return rc;
	}
	
	/* Read the DmControl.DmActive bit */
	dmactive = VT1_GET_FIELD(val,
				 VT1_DBMD_DMCONTROL_DMACTIVE_MSK,
				 VT1_DBMD_DMCONTROL_DMACTIVE_POS);
	/* Read on DmControl.DmActive == 1 means Debug Module is enabled */
	if (dmactive) {
		vt1_info("Debug Module already enabled");
		return VMSPLAT_ERR_EALREADY;
	}
	
	/* Write DmControl.DmActive = 1 to start the DM activation sequence */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     VT1_REG_32(dbmd_base, VT1_DBMD_DMCONTROL_OFFSET),
			     VT1_DBMD_DMCONTROL_DMACTIVE_MSK,
			     VT1_DBMD_DMCONTROL_DMACTIVE_POS,
			     1);
	if (rc) {
		vt1_error("Write DBMD DmControl.DmActive: %d", rc);
		return rc;
	}
	
	val = 0;
	/* DmControl.DmActive = 1 drives the DmActiveControlStatus.DmActiveRequestedState == 1 */
	/* Poll for DmActiveControlStatus.DmactiveRequestedState == 1 */
	while (!dmactive) {
		rc = vt1_read(apcluster_idx,
			      VT1_REG_32(dbmd_base, VT1_DBMD_DMACTIVECONTROLSTATUS_OFFSET),
			      &val);
		if (rc) {
			vt1_error("Read DBMD DmActiveControlStatus Register: %d", rc);
			return rc;
		}

		dmactive = VT1_GET_FIELD(val,
					 VT1_DBMD_DMACTIVECONTROLSTATUS_DMACTIVEREQUESTEDSTATE_MSK,
					 VT1_DBMD_DMACTIVECONTROLSTATUS_DMACTIVEREQUESTEDSTATE_POS);
	}

	/* Checkpoint: DmControl.DmActive == 1,
	 * DmActiveControlStatus.DmActiveRequestedState == 1
	 *
	 * Write the DmActiveControlStatus.DmActiveCurrentState = 1 */
	val = 0;
	VT1_SET_FIELD(val, VT1_DBMD_DMACTIVECONTROLSTATUS_DMACTIVECURRENTSTATE_MSK,
		      VT1_DBMD_DMACTIVECONTROLSTATUS_DMACTIVECURRENTSTATE_POS,
		      1);
	
	rc = vt1_write(apcluster_idx,
		       VT1_REG_32(dbmd_base, VT1_DBMD_DMACTIVECONTROLSTATUS_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to write DbmdActiveControlStatus");
		return VMSPLAT_ERR_EIO;
	}
	
	/* 20 sec delay to let the DM status gets reflected into
	 * DmControl.DmActive. The delay is arbitrary and can be
	 * fine tuned if required */
	vt1_nsdelay(20);
	
	val = 0;
	rc = vt1_read(apcluster_idx,
		      VT1_REG_32(dbmd_base, VT1_DBMD_DMACTIVECONTROLSTATUS_OFFSET),
		      &val);
	if (rc) {
		vt1_error("Read DBMD DmActiveControlStatus Register: %d", rc);
		return rc;
	}

	vt1_info("DmActiveControlStatus[%p] Register Read Value: %x",
		(void *)VT1_REG_32(dbmd_base, VT1_DBMD_DMACTIVECONTROLSTATUS_OFFSET), val);
	
	val = 0;
	rc = vt1_read(apcluster_idx,
		      VT1_REG_32(dbmd_base, VT1_DBMD_DMCONTROL_OFFSET),
		      &val);
	if (rc) {
		vt1_error("Read DBMD DmControl Register: %d", rc);
		return rc;
	}

	vt1_info("DmControl[%p] Register Read Value: %x",
		(void *)VT1_REG_32(dbmd_base, VT1_DBMD_DMCONTROL_OFFSET), val);

	/* Checkpoint: DM is enabled. Enable harts visibility to debugger */
	rc = vt1_dbmd_harts_enable(apcluster_idx, dbmd_base, num_harts);
	if (rc) {
		vt1_error("Failed to enable harts in HartAvailableStatus - %d", rc);
	}

	return rc;
}
