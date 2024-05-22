/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#define vt1_fmt(fmt) "PCL: " fmt
#define DEBUG
#include <vt1/vt1_pcl_init.h>

#define VT1_PMA_BASE		VT1_CPU_PMACFG0_OFFSET
#define VT1_PMA_STRIDE		(VT1_CPU_PMACFG1_OFFSET - VT1_PMA_BASE)
#define VT1_PMACFG_NUM		(VT1_CPU_PMACFG0_OFFSET - VT1_PMA_BASE)
#define VT1_PMAADDRLO_NUM	(VT1_CPU_PMAADDRLO0_OFFSET- VT1_PMA_BASE)
#define VT1_PMAADDRHI_NUM	(VT1_CPU_PMAADDRHI0_OFFSET- VT1_PMA_BASE)

#define vt1_get_pmacfg_offset(__i)	\
	(VT1_PMA_BASE + ((__i) * VT1_PMA_STRIDE) + VT1_PMACFG_NUM)
#define vt1_get_pmalo_offset(__i)	\
	(VT1_PMA_BASE + ((__i) * VT1_PMA_STRIDE) + VT1_PMAADDRLO_NUM)
#define vt1_get_pmahi_offset(__i)	\
	(VT1_PMA_BASE + ((__i) * VT1_PMA_STRIDE) + VT1_PMAADDRHI_NUM)

static int vt1_cpu_configure_pma(vmsplat_uint32_t apcluster_idx,
				 vmsplat_uint32_t cpu,
				 vmsplat_uint32_t num_pmaregion,
				 const struct vmsplat_config_pmaregion *pmaregion,
				 vmsplat_uint64_t pcrb_base)
{
	vmsplat_uint32_t val = 0;
	vmsplat_uint64_t pma_addr, addr, pma_addr_mask;
	vmsplat_uint32_t pma_addr_lo = 0, pma_addr_hi = 0;
	vmsplat_size_t i;
	int rc;

	if (num_pmaregion > VT1_PMA_ENTRIES_MAX) {
		vt1_error("PMA entries/regions provided more than supported");
		return VMSPLAT_ERR_EINVALID;
	}

	for (i = 0; i < num_pmaregion; i++) {
		switch (pmaregion[i].type) {
			case VMSPLAT_PMAREGION_TYPE_WB:
				val = VT1_PMA_TYPE_WB;
				rc = vt1_write(apcluster_idx,
					       CPU_MMR(pcrb_base, cpu, vt1_get_pmacfg_offset(i)),
					       val);
				if (rc) {
					vt1_error("Failed to write to PMACFG[%lu]", i);
					return rc;
				}
				break;

			case VMSPLAT_PMAREGION_TYPE_WC:
				val = VT1_PMA_TYPE_WC;
				rc = vt1_write(apcluster_idx,
					       CPU_MMR(pcrb_base, cpu, vt1_get_pmacfg_offset(i)),
					       val);
				if (rc) {
					vt1_error("Failed to write to PMACFG[%lu]", i);
					return rc;
				}
				break;

			case VMSPLAT_PMAREGION_TYPE_UC:
				val = VT1_PMA_TYPE_UC;
				rc = vt1_write(apcluster_idx,
					       CPU_MMR(pcrb_base, cpu, vt1_get_pmacfg_offset(i)),
					       val);
				if (rc) {
					vt1_error("Failed to write to PMACFG[%lu]", i);
					return rc;
				}
				break;

			case VMSPLAT_PMAREGION_TYPE_VACANT:
				val = VT1_PMA_TYPE_VACANT;
				rc = vt1_write(apcluster_idx,
					       CPU_MMR(pcrb_base, cpu, vt1_get_pmacfg_offset(i)),
					       val);
				if (rc) {
					vt1_error("Failed to write to PMACFG[%lu]", i);
					return rc;
				}
				break;

			default:
				vt1_error("Invalid PMA region type");
				return VMSPLAT_ERR_EINVALID;
		}

		/* NAPOT ranges */
		addr = pmaregion[i].addr;
		pma_addr_mask = (_ULL(1) << (pmaregion[i].order - VT1_PMA_SHIFT)) - 1;
		pma_addr = ((addr >> VT1_PMP_SHIFT) | pma_addr_mask);

		/* Configure PMADDRHI */
		val = 0;
		pma_addr_hi = (pma_addr >> 32) & 0xffffffff;
		VT1_SET_FIELD(val, VT1_CPU_PMAADDRHI0_ADDRESS_MSK,
				  VT1_CPU_PMAADDRHI0_ADDRESS_POS,
				  pma_addr_hi);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, vt1_get_pmahi_offset(i)),
			       val);
		if (rc) {
			vt1_error("Failed to write to PMAADDRHI[%lu]", i);
			return VMSPLAT_ERR_EIO;
		}

		/* Configure PMADDRLO */
		val = 0;
		pma_addr_lo = pma_addr & 0xffffffff;

		VT1_SET_FIELD(val, VT1_CPU_PMAADDRLO0_ADDRESS_MSK,
				  VT1_CPU_PMAADDRLO0_ADDRESS_POS,
				  pma_addr_lo);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, vt1_get_pmalo_offset(i)),
			       val);
		if (rc) {
			vt1_error("Failed to write to PMAADDRLO[%lu]", i);
			return VMSPLAT_ERR_EIO;
		}

	}
	vt1_debug("Cluster-%d: PMA configured for cpu - %d", apcluster_idx, cpu);
	return VMSPLAT_OK;
}

/* TODO: Only 2-CPU config handled, make it generic */
static int vt1_sbr_credits_config(vmsplat_uint32_t apcluster_idx,
				  vmsplat_uint32_t pcrb_base)
{
	int rc;
	vmsplat_uint32_t val = 0;

	/* (SBR-ICS0) SnpCredits0 */
	VT1_SET_FIELD(val,
		      VT1_SBR_SNPCREDITS0_MAXCREDITS_MSK,
		      VT1_SBR_SNPCREDITS0_MAXCREDITS_POS,
		      VT1_SBR_SNPCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       SBR_MMR(pcrb_base, VT1_SBR_SNPCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Sbr SnpCredits0.MaxCredits: Cluster-%u: %d",
			  apcluster_idx, rc);
		return rc;
	}

	/* (SBR-ICS0) DatCredits0 */
	val = 0;
	VT1_SET_FIELD(val,
		      VT1_SBR_DATCREDITS0_MAXCREDITS_MSK,
		      VT1_SBR_DATCREDITS0_MAXCREDITS_POS,
		      VT1_SBR_DATCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       SBR_MMR(pcrb_base, VT1_SBR_DATCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Sbr DatCredits0.MaxCredits: Cluster-%u: %d",
			  apcluster_idx, rc);
		return rc;
	}

	/* (SBR-ICS0) RspCredits0 */
	val = 0;
	VT1_SET_FIELD(val,
		      VT1_SBR_RSPCREDITS0_MAXCREDITS_MSK,
		      VT1_SBR_RSPCREDITS0_MAXCREDITS_POS,
		      VT1_SBR_RSPCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       SBR_MMR(pcrb_base, VT1_SBR_RSPCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Sbr RspCredits0.MaxCredits: Cluster-%u: %d",
			  apcluster_idx, rc);
		return rc;
	}

	return VMSPLAT_OK;
}

/* TODO: Only 2-CPU config handled, make it generic */
static int vt1_ics_credits_config(vmsplat_uint32_t apcluster_idx,
				  vmsplat_uint32_t cpu,
				  vmsplat_uint32_t pcrb_base)
{
	int rc;
	vmsplat_uint32_t val = 0;

	/* (ICS0-ICS0) ReqCredits0 */
	VT1_SET_FIELD(val,
		      VT1_ICS_ICSREQCREDITS0_MSK,
		      VT1_ICS_ICSREQCREDITS0_POS,
		      VT1_ICS_ICSREQCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       ICS_MMR(pcrb_base, cpu, VT1_ICS_ICSREQCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Ics IcsReqCredits0.MaxCredits: Cpu-%u: %d",
			  cpu, rc);
		return rc;
	}
	
	VT1_SET_FIELD(val,
		      VT1_ICS_SBRREQCREDITS_MSK,
		      VT1_ICS_SBRREQCREDITS_POS,
		      VT1_ICS_SBRREQCREDITS_CREDITS);
	rc = vt1_write(apcluster_idx,
		       ICS_MMR(pcrb_base, cpu, VT1_ICS_SBRREQCREDITS_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Ics SbrReqCredits.MaxCredits: Cpu-%u: %d",
			  cpu, rc);
		return rc;
	}

	/* (ICS0-ICS0) SnpCredits0 */
	val = 0;
	VT1_SET_FIELD(val,
		      VT1_ICS_SNPCREDITS0_MAXCREDITS_MSK,
		      VT1_ICS_SNPCREDITS0_MAXCREDITS_POS,
		      VT1_ICS_SNPCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       ICS_MMR(pcrb_base, cpu, VT1_ICS_SNPCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Ics SnpCredits0.MaxCredits: Cpu-%u: %d",
			  cpu, rc);
		return rc;
	}
	
	/* (ICS0-ICS0) DatCredits0 */
	val = 0;
	VT1_SET_FIELD(val,
		      VT1_ICS_DATCREDITS0_MAXCREDITS_MSK,
		      VT1_ICS_DATCREDITS0_MAXCREDITS_POS,
		      VT1_ICS_DATCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       ICS_MMR(pcrb_base, cpu, VT1_ICS_DATCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Ics DatCredits0.MaxCredits: Cpu-%u: %d",
			  cpu, rc);
		return rc;
	}

	/* (ICS0-ICS0) RspCredits0 */
	val = 0;
	VT1_SET_FIELD(val,
		      VT1_ICS_RSPCREDITS0_MAXCREDITS_MSK,
		      VT1_ICS_RSPCREDITS0_MAXCREDITS_POS,
		      VT1_ICS_RSPCREDITS0_CREDITS);
	rc = vt1_write(apcluster_idx,
		       ICS_MMR(pcrb_base, cpu, VT1_ICS_RSPCREDITS0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Setting Ics RspCredits0.MaxCredits: Cpu-%u: %d",
			  cpu, rc);
		return rc;
	}

	return VMSPLAT_OK;
}

int vt1_cpu_ras_dump_state(vmsplat_uint32_t apcluster_idx,
			   vmsplat_uint32_t cpu,
			   vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t val;
	
	rc = vt1_read(apcluster_idx,
		      CPU_MMR(pcrb_base, cpu, VT1_CPU_RASBANK0STATUS_OFFSET),
		      &val);
	if (rc) {
		vt1_error("Read RasBank0Status: CPU-%u: %d", cpu, rc);
		return rc;
	}

	vt1_info("RasBank0Status: %x", val);

	/* RasBank1 - RasBank11 */
	for (i = 0; i < 11; i++) {
		/* RasBankXStatus Register */
		rc = vt1_read(apcluster_idx,
			      CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1STATUS_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading RasBankXStatus: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
		
		vt1_info("RasBank%dStatus: %x", i, val);
		
		/* RasBankXAddrL Register */
		rc = vt1_read(apcluster_idx,
			      CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1ADDRL_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading RasBankXAddrL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		vt1_info("RasBank%dAddrL: %x", i, val);

		/* RasBankXAddrH Register */
		rc = vt1_read(apcluster_idx,
			      CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1ADDRH_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading RasBankXAddrH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
		
		vt1_info("RasBank%dAddrH: %x", i, val);

		/* RasBankXInfoL Register */
		rc = vt1_read(apcluster_idx,
			      CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1INFOL_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading RasBankXInfoL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		vt1_info("RasBank%dInfoL: %x", i, val);

		/* RasBankXInfoH Register */
		rc = vt1_read(apcluster_idx,
			      CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1INFOH_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading RasBankXInfoH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		vt1_info("RasBank%dInfoH: %x", i, val);
	}

	return VMSPLAT_OK;
}

int vt1_cpu_ras_clear_state(vmsplat_uint32_t apcluster_idx,
			    vmsplat_uint32_t cpu,
			    vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t val = 0;

	/* RasBank0.RecentErr */
	VT1_CLEAR_FIELD(val, VT1_CPU_RASBANK0STATUS_RECENTERR_MSK,
		        VT1_CPU_RASBANK0STATUS_RECENTERR_POS);

	rc = vt1_write(apcluster_idx,
		       CPU_MMR(pcrb_base, cpu, VT1_CPU_RASBANK0STATUS_OFFSET),
		       val);
	if (rc) {
		vt1_error("Clearing RasBank0Status.RecentErr: CPU-%u: %d", cpu, rc);
		return rc;
	}

	/* RasBank1 - RasBank11 */
	for (i = 0; i < 11; i++) {
		/* RasBankXStatus Register */
		VT1_CLEAR_FIELD(val, VT1_CPU_RASBANK1STATUS_MSK, VT1_CPU_RASBANK1STATUS_POS);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1STATUS_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Clearing RasBankXStatus: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* RasBankXAddrL Register */
		VT1_CLEAR_FIELD(val, VT1_CPU_RASBANK1ADDRL_MSK, VT1_CPU_RASBANK1ADDRL_POS);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1ADDRL_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Clearing RasBankXAddrL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* RasBankXAddrH Register */
		VT1_CLEAR_FIELD(val, VT1_CPU_RASBANK1ADDRH_MSK, VT1_CPU_RASBANK1ADDRH_POS);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1ADDRH_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Clearing RasBankXAddrH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* RasBankXInfoL Register */
		VT1_CLEAR_FIELD(val, VT1_CPU_RASBANK1INFOL_MSK, VT1_CPU_RASBANK1INFOL_POS);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1INFOL_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Clearing RasBankXInfoL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* RasBankXInfoH Register */
		VT1_CLEAR_FIELD(val, VT1_CPU_RASBANK1INFOH_MSK, VT1_CPU_RASBANK1INFOH_POS);
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1INFOH_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Clearing RasBankXInfoH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

int vt1_cpu_ras_interrupts_enable(vmsplat_uint32_t apcluster_idx,
				  vmsplat_uint32_t cpu,
				  vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t val = 0;

	VT1_SET_FIELD(val, VT1_CPU_RASBANK0CTRL_NONCRITERRINTREN_MSK,
		      VT1_CPU_RASBANK0CTRL_NONCRITERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_CPU_RASBANK0CTRL_FATALERRINTREN_MSK,
		      VT1_CPU_RASBANK0CTRL_FATALERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_CPU_RASBANK0CTRL_NMIEN_MSK,
		      VT1_CPU_RASBANK0CTRL_NMIEN_POS, 1);

	rc = vt1_write(apcluster_idx,
		       CPU_MMR(pcrb_base, cpu, VT1_CPU_RASBANK0CTRL_OFFSET),
		       val);
	if (rc) {
		vt1_error("Enable RasBank0Ctl Interrupts: CPU-%u: %d", cpu, rc);
		return rc;
	}

	val = 0;

	VT1_SET_FIELD(val, VT1_CPU_RASBANK1CTRL_NONCRITCERRINTREN_MSK,
		      VT1_CPU_RASBANK1CTRL_NONCRITCERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_CPU_RASBANK1CTRL_NONCRITDERRINTREN_MSK,
		      VT1_CPU_RASBANK1CTRL_NONCRITDERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_CPU_RASBANK1CTRL_FATALERRINTREN_MSK,
		      VT1_CPU_RASBANK1CTRL_FATALERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_CPU_RASBANK1CTRL_NMIEN_MSK,
		      VT1_CPU_RASBANK1CTRL_NMIEN_POS, 1);

	for (i = 0; i < 11; i++) {
		rc = vt1_write(apcluster_idx,
			       CPU_MMR(pcrb_base, cpu, (VT1_CPU_RASBANK1CTRL_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Enable RasBankXCtl Interrupts: CPU-%u: RasBank:-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

int vt1_l3m_ras_dump_state(vmsplat_uint32_t apcluster_idx,
			   vmsplat_uint32_t cpu,
			   vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t val = 0;
	
	/* RasBank0 */
	rc = vt1_read(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_L3MRASBANK0STATUS_OFFSET),
			     &val);
	if (rc) {
		vt1_error("Reading L3MRasBank0Status.RecentErr: CPU-%u: %d", cpu, rc);
		return rc;
	}
	
	vt1_info("L3MRasBank0Status: %x", val);

	/* RasBank1 - RasBank5 */
	for (i = 0; i < 5; i++) {
		/* L3MRasBankXStatus Register */
		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1STATUS_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading L3MRasBankXStatus: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
		
		vt1_info("L3MRasBank%dStatus: %x", i, val);

		/* L3MRasBankXAddrL Register */
		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1ADDRL_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading L3MRasBankXAddrL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
		
		vt1_info("L3MRasBank%dAddrL: %x", i, val);

		/* L3MRasBankXAddrH Register */
		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1ADDRH_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading L3MRasBankXAddrH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		vt1_info("L3MRasBank%dAddrH: %x", i, val);

		/* L3MRasBankXInfoL Register */
		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1INFOL_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading L3MRasBankXInfoL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
		
		vt1_info("L3MRasBank%dInfoL: %x", i, val);

		/* L3MRasBankXInfoH Register */
		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1INFOH_OFFSET + i*8)),
			      &val);
		if (rc) {
			vt1_error("Reading L3MRasBankXInfoH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
		
		vt1_info("L3MRasBank%dInfoH: %x", i, val);

	}

	return VMSPLAT_OK;
}

int vt1_l3m_ras_clear_state(vmsplat_uint32_t apcluster_idx,
			    vmsplat_uint32_t cpu,
			    vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	
	/* RasBank0 */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_L3MRASBANK0STATUS_OFFSET),
			     VT1_L3M_L3MRASBANK0STATUS_RECENTERR_MSK,
			     VT1_L3M_L3MRASBANK0STATUS_RECENTERR_POS,
			     0);
	if (rc) {
		vt1_error("Clearing L3MRasBank0Status.RecentErr: CPU-%u: %d", cpu, rc);
		return rc;
	}
	
	/* RasBank1 - RasBank5 */
	for (i = 0; i < 5; i++) {
		/* L3MRasBankXStatus Register */
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1STATUS_OFFSET + i*8)),
				     VT1_L3M_L3MRASBANK1STATUS_MSK,
				     VT1_L3M_L3MRASBANK1STATUS_POS,
				     0);
		if (rc) {
			vt1_error("Clearing L3MRasBankXStatus: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* L3MRasBankXAddrL Register */
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1ADDRL_OFFSET + i*8)),
				     VT1_L3M_L3MRASBANK1ADDRL_MSK,
				     VT1_L3M_L3MRASBANK1ADDRL_POS,
				     0);
		if (rc) {
			vt1_error("Clearing L3MRasBankXAddrL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* L3MRasBankXAddrH Register */
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1ADDRH_OFFSET + i*8)),
				     VT1_L3M_L3MRASBANK1ADDRH_MSK,
				     VT1_L3M_L3MRASBANK1ADDRH_POS,
				     0);
		if (rc) {
			vt1_error("Clearing L3MRasBankXAddrH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* L3MRasBankXInfoL Register */
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1INFOL_OFFSET + i*8)),
				     VT1_L3M_L3MRASBANK1INFOL_MSK,
				     VT1_L3M_L3MRASBANK1INFOL_POS,
				     0);
		if (rc) {
			vt1_error("Clearing L3MRasBankXInfoL: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/* L3MRasBankXInfoH Register */
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1INFOH_OFFSET + i*8)),
				     VT1_L3M_L3MRASBANK1INFOH_MSK,
				     VT1_L3M_L3MRASBANK1INFOH_POS,
				     0);
		if (rc) {
			vt1_error("Clearing L3MRasBankXInfoH: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

int vt1_l3m_ras_interrupts_enable(vmsplat_uint32_t apcluster_idx,
				  vmsplat_uint32_t cpu,
				  vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t val = 0;

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK0CTRL_NONCRITERRINTREN_MSK,
		      VT1_L3M_L3MRASBANK0CTRL_NONCRITERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK0CTRL_FATALERRINTREN_MSK,
		      VT1_L3M_L3MRASBANK0CTRL_FATALERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK0CTRL_NMIEN_MSK,
		      VT1_L3M_L3MRASBANK0CTRL_NMIEN_POS, 1);

	rc = vt1_write(apcluster_idx,
		       L3M_MMR(pcrb_base, cpu, VT1_L3M_L3MRASBANK0CTRL_OFFSET),
		       val);
	if (rc) {
		vt1_error("Enable RasBank0Ctl Interrupts: CPU-%u: %d", cpu, rc);
		return rc;
	}

	val = 0;

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK1CTRL_NONCRITCERRINTREN_MSK,
		      VT1_L3M_L3MRASBANK1CTRL_NONCRITCERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK1CTRL_NONCRITDERRINTREN_MSK,
		      VT1_L3M_L3MRASBANK1CTRL_NONCRITDERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK1CTRL_FATALERRINTREN_MSK,
		      VT1_L3M_L3MRASBANK1CTRL_FATALERRINTREN_POS, 1);

	VT1_SET_FIELD(val, VT1_L3M_L3MRASBANK1CTRL_NMIEN_MSK,
		      VT1_L3M_L3MRASBANK1CTRL_NMIEN_POS, 1);

	for (i = 0; i < 5; i++) {
		rc = vt1_write(apcluster_idx,
			       L3M_MMR(pcrb_base, cpu, (VT1_L3M_L3MRASBANK1CTRL_OFFSET + i*8)),
			       val);
		if (rc) {
			vt1_error("Enable L3MRasBankXCtl Interrupts: CPU-%u: RasBank:-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

static int vt1_cpu_ram_init(vmsplat_uint32_t apcluster_idx,
			    vmsplat_uint32_t cpu,
			    vmsplat_uint64_t pcrb_base)
{
	vmsplat_uint32_t val, ram_init_done = 0;
	int rc;

	/* Core RAM initialization */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
			     VT1_L3M_CPURSTCTL_STARTRAMINIT_MSK,
			     VT1_L3M_CPURSTCTL_STARTRAMINIT_POS,
			     1);
	if (rc) {
		vt1_error("Set CpuRstCtl.StartRamInit: CPU-%u: %d", cpu, rc);
		return rc;
	}

	/* Poll until Core RAM Init Done */
	while (!ram_init_done) {
		vt1_nsdelay(VT1_RAMINIT_DELAY);

		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
			      &val);
		if (rc) {
			vt1_error("Failed to read CpuRstCtl[%d]", cpu);
			return VMSPLAT_ERR_EIO;
		}
		ram_init_done =  VT1_GET_FIELD(val,
					      VT1_L3M_CPURSTCTL_RAMINTDONE_MSK,
					      VT1_L3M_CPURSTCTL_RAMINTDONE_POS);
	}
	
	/**
	 * VT1-ERRATUM:
	 * https://gitlab.dc1.ventanamicro.com/ventana/mono/-/issues/14465
	 * Currently HW does not clear the StartRamInit upon asserting
	 * the RamInitDone
	 **/
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
			     VT1_L3M_CPURSTCTL_STARTRAMINIT_MSK,
			     VT1_L3M_CPURSTCTL_STARTRAMINIT_POS,
			     0);
	if (rc) {
		vt1_error("Clear CpuRstCtl.StartRamInit: CPU-%u: %d", cpu, rc);
		return rc;
	}

	vt1_debug("Cluster-%d: CPU RAM Init done for cpu - %d", apcluster_idx, cpu);
	return VMSPLAT_OK;
}

int vt1_cpu_start(vmsplat_uint32_t apcluster_idx,
		  vmsplat_uint32_t cpu,
		  vmsplat_uint64_t pcrb_base)
{
	int rc = 0;
	
	rc = vt1_cpu_ras_interrupts_enable(apcluster_idx, cpu, pcrb_base);
	if (rc) {
		vt1_error("Failed to enable CPU RAS Interrupts: CPU-%u: %d", cpu, rc);
		return rc;
	}

	vt1_l3m_ras_interrupts_enable(apcluster_idx, cpu, pcrb_base);
	if (rc) {
		vt1_error("Failed to enable L3M RAS Interrupts: CPU-%u: %d", cpu, rc);
		return rc;
	}

	/* CpuRstCtl[i].Start */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
			     VT1_L3M_CPURSTCTL_START_MSK,
			     VT1_L3M_CPURSTCTL_START_POS,
			     1);
	if (rc) {
		vt1_error("Failed to write Start to CpuRstCtl[%d]", cpu);
	}

	return rc;
}

int vt1_cpu_configure_amc(vmsplat_uint32_t apcluster_idx,
			  vmsplat_uint32_t cpu,
			  vmsplat_uint64_t pcrb_base,
			  vmsplat_uint32_t *pwr_coeffs,
			  vmsplat_uint32_t pwr_sample_interval,
			  vmsplat_uint32_t *perf_coeffs,
			  vmsplat_uint32_t perf_sample_interval,
			  vmsplat_uint32_t noncore_perf_sample_interval)
{
	int rc = 0, i;

	/* Program AMC Core Power Estimation Coefficients */
	for (i = 0; i < VT1_PCL_AMCPWR_COEFF_MAX_IDX; i++) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
			CPU_MMR(pcrb_base, cpu, VT1_CPU_AMCPWRCOEFF0_OFFSET + i),
			VT1_CPU_AMCPWRCOEFF0_COEFF_MSK,
			VT1_CPU_AMCPWRCOEFF0_COEFF_POS,
			pwr_coeffs ? pwr_coeffs[i] : 0);
		if (rc)
			return rc;
	}

	/* Program AMC Core Power Estimation Sampling Interval */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			CPU_MMR(pcrb_base, cpu, VT1_CPU_AMCPWRCTL_OFFSET),
			VT1_CPU_AMCPWRCTL_SAMPLINT_MSK,
			VT1_CPU_AMCPWRCTL_SAMPLINT_POS,
			pwr_sample_interval);
	if (rc)
		return rc;

	/* Program AMC Core Performance Estimation Cofficients */
	for (i = 0; i < VT1_PCL_AMCPERF_COEFF_MAX_IDX; i++) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
			CPU_MMR(pcrb_base, cpu, VT1_CPU_AMCPERFCOEFF0_OFFSET + i),
			VT1_CPU_AMCPERFCOEFF0_COEFF_MSK,
			VT1_CPU_AMCPERFCOEFF0_COEFF_POS,
			perf_coeffs ? perf_coeffs[i] : 0);
		if (rc)
			return rc;
	}

	/* Program AMC Core Performance Estimation Sampling Interval */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			CPU_MMR(pcrb_base, cpu, VT1_CPU_AMCPERFCTL_OFFSET),
			VT1_CPU_AMCPERFCTL_SAMPLINT_MSK,
			VT1_CPU_AMCPERFCTL_SAMPLINT_POS,
			perf_sample_interval);
	if (rc)
		return rc;

	/* Program AMC Non-Core(L3M) Performance Sampling Interval */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			CPU_MMR(pcrb_base, cpu, VT1_CPU_AMCNONCOREPERFCTL_OFFSET),
			VT1_CPU_AMCNONCOREPERFCTL_SAMPLINT_MSK,
			VT1_CPU_AMCNONCOREPERFCTL_SAMPLINT_POS,
			noncore_perf_sample_interval);
	if (rc)
		return rc;

	return VMSPLAT_OK;
}


int vt1_cpu_l3m_ram_init(vmsplat_uint32_t apcluster_idx,
			 vmsplat_uint32_t cpu,
			 vmsplat_uint64_t pcrb_base,
			 vmsplat_uint8_t *pcl_cap)
{
	vmsplat_uint32_t val, ram_init_done;
	int rc;
	
	if (!pcl_cap) {
		vt1_error("Invalid PCL Capability configuration\n");
		return VMSPLAT_ERR_EINVALID;
	}

	rc = vt1_read(apcluster_idx,
		      L3M_MMR(pcrb_base, cpu, VT1_L3M_UL3CONFIG1_OFFSET),
		      &val);
	if (rc) {
		vt1_error("Failed to read Ul3Config1[%d] - %d", cpu, rc);
		return VMSPLAT_ERR_EIO;
	}
	
	/* Enable Snoops - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_SNOOP] < VT1_PCL_CAP_VALUE_MAX_IDX)
		VT1_SET_FIELD(val,
			      VT1_L3M_UL3CONFIG1_SNPFILTERDIS_MSK,
			      VT1_L3M_UL3CONFIG1_SNPFILTERDIS_POS,
			      pcl_cap[VT1_PCL_CAP_SNOOP]? 0 : 1);

	/* Enable Cache - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_CACHE] < VT1_PCL_CAP_VALUE_MAX_IDX)
		VT1_SET_FIELD(val,
			      VT1_L3M_UL3CONFIG1_CACHEDIS_MSK,
			      VT1_L3M_UL3CONFIG1_CACHEDIS_POS,
			      pcl_cap[VT1_PCL_CAP_CACHE]? 0 : 1);

	rc = vt1_write(apcluster_idx,
		       L3M_MMR(pcrb_base, cpu, VT1_L3M_UL3CONFIG1_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to Enable Snoops and Cache in Ul3Config1[%d] - %d", cpu, rc);
		return VMSPLAT_ERR_EIO;
	}

	/* L3M RAM Initialization */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_L3MCFGINIT_OFFSET),
			     VT1_L3M_L3MCFGINIT_STARTRAMINIT_MSK,
			     VT1_L3M_L3MCFGINIT_STARTRAMINIT_POS,
			     1);
	if (rc) {
		vt1_error("Failed to program L3MConfig[%d]", cpu);
		return rc;
	}

	/* Poll until L3M RAM Init Done */
	ram_init_done = 0;
	while (!ram_init_done) {
		vt1_nsdelay(VT1_RAMINIT_DELAY);

		rc = vt1_read(apcluster_idx,
			      L3M_MMR(pcrb_base, cpu, VT1_L3M_L3MCFGINIT_OFFSET),
			      &val);
		if (rc) {
			vt1_error("Failed to read L3MConfig[%d]", cpu);
			return VMSPLAT_ERR_EIO;
		}

		ram_init_done = VT1_GET_FIELD(val,
					  VT1_L3M_L3MCFGINIT_RAMINTDONE_MSK,
					  VT1_L3M_L3MCFGINIT_RAMINTDONE_POS);
	}
	
	rc = vt1_l3m_ras_clear_state(apcluster_idx, cpu, pcrb_base);
	if (rc) {
		vt1_error("Failed to clear the L3M RAS Banks State");
		return rc;
	}

	vt1_debug("Cluster-%d: L3M RAM Init done for cpu-%d ", apcluster_idx, cpu);
	return VMSPLAT_OK;
}

static int vt1_cpu_capability_config(vmsplat_uint32_t apcluster_idx,
				     vmsplat_uint32_t cpu,
				     vmsplat_uint64_t pcrb_base,
				     vmsplat_uint8_t *pcl_cap)
{
	int rc;
	
	/* Enable AIA - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_AIA] < VT1_PCL_CAP_VALUE_MAX_IDX) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     CPU_MMR(pcrb_base, cpu, VT1_CPU_DECCONFIG_OFFSET),
				     VT1_CPU_DECCONFIG_AIADIS_MSK,
				     VT1_CPU_DECCONFIG_AIADIS_POS,
				     pcl_cap[VT1_PCL_CAP_AIA]? 0 : 1);
		
		if (rc) {
			vt1_error("Failed to set AIA CAP: DecConfig[%d]", cpu);
			return rc;
		}
	}
		
	/* Enable CBO - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_CBO] < VT1_PCL_CAP_VALUE_MAX_IDX) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     CPU_MMR(pcrb_base, cpu, VT1_CPU_DECCONFIG_OFFSET),
				     VT1_CPU_DECCONFIG_RVCBODIS_MSK,
				     VT1_CPU_DECCONFIG_RVCBODIS_POS,
				     pcl_cap[VT1_PCL_CAP_CBO]? 0 : 1);
	
		if (rc) {
			vt1_error("Failed to set CBO CAP: DecConfig[%d]", cpu);
			return rc;
		}
	}

	/* Enable ZBABCS - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_ZABCS] < VT1_PCL_CAP_VALUE_MAX_IDX) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     CPU_MMR(pcrb_base, cpu, VT1_CPU_DECCONFIG2_OFFSET),
				     VT1_CPU_DECCONFIG2_BABCSDIS_MSK,
				     VT1_CPU_DECCONFIG2_BABCSDIS_POS,
				     pcl_cap[VT1_PCL_CAP_ZABCS]? 0 : 1);
		
		if (rc) {
			vt1_error("Failed to set ZBABCS CAP: DecConfig2[%d]", cpu);
			return rc;
		}
	}
	
	/* Enable SVINVAL - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_SVINVAL] < VT1_PCL_CAP_VALUE_MAX_IDX) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     CPU_MMR(pcrb_base, cpu, VT1_CPU_DECCONFIG2_OFFSET),
				     VT1_CPU_DECCONFIG2_SVINVALDIS_MSK,
				     VT1_CPU_DECCONFIG2_SVINVALDIS_POS,
				     pcl_cap[VT1_PCL_CAP_SVINVAL]? 0 : 1);
		
		if (rc) {
			vt1_error("Failed to set SVINVAL CAP: DecConfig2[%d]", cpu);
			return rc;
		}
	}
	
	/* Enable VCCONDOPS - 0 for Enable, 1 for Disable */
	if (pcl_cap[VT1_PCL_CAP_VSCONDOPS] < VT1_PCL_CAP_VALUE_MAX_IDX) {
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     CPU_MMR(pcrb_base, cpu, VT1_CPU_DECCONFIG2_OFFSET),
				     VT1_CPU_DECCONFIG2_VCCONDOPSDIS_MSK,
				     VT1_CPU_DECCONFIG2_VCCONDOPSDIS_POS,
				     pcl_cap[VT1_PCL_CAP_VSCONDOPS]? 0 : 1);
		if (rc) {
			vt1_error("Failed to set VCCONDOPS CAP: DecConfig2[%d]", cpu);
			return rc;
		}
	}
	return VMSPLAT_OK;
}

int vt1_cpu_init(vmsplat_uint32_t apcluster_idx,
		 vmsplat_uint16_t cpu,
		 vmsplat_uint64_t zstage_load_addr,
		 vmsplat_uint8_t *pcl_cap,
		 vmsplat_uint32_t num_pmaregion,
		 const struct vmsplat_config_pmaregion *pmaregion,
		 vmsplat_uint64_t pcrb_base)
{
	int rc = 0;
	vmsplat_uint32_t rvec_addr_lo, rvec_addr_hi, val = 0;
	
	if (!pcl_cap) {
		vt1_error("Invalid PCL Capability configuration\n");
		return VMSPLAT_ERR_EINVALID;
	}

	/*
	 * REVISIT: Should we poll for VT1_L3M_CPURSTCTL_HARDRESETDONE
	 * after clearing VT1_L3M_CPURSTCTL_HARDRESET?
	 */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
			     VT1_L3M_CPURSTCTL_HARDRESET_MSK,
			     VT1_L3M_CPURSTCTL_HARDRESET_POS,
			     0);
	if (rc) {
		vt1_error("Failed to program CpuRstCtl[%d]", cpu);
		return rc;
	}

	/* Enable MMR access */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
			     VT1_L3M_CPURSTCTL_REGBUSENABLE_MSK,
			     VT1_L3M_CPURSTCTL_REGBUSENABLE_POS,
			     1);
	if (rc) {
		vt1_error("Failed to set CpuRstCtl[%d].RegBusEnable", cpu);
		return rc;
	}
	
	rc = vt1_cpu_capability_config(apcluster_idx, cpu, pcrb_base, pcl_cap);
	if (rc) {
		vt1_error("Cluster[%d] - Failed to program cpu[%d] capabilities\n",
			  apcluster_idx, cpu);
		return rc;
	}

	/* Configure Reset and NMI Vector */
	rvec_addr_lo = (zstage_load_addr & 0xffffffff) >> 6; /* Should be 64B aligned */
	rvec_addr_hi = (zstage_load_addr >> 32) & 0xffffffff;

	val = 0;
	VT1_SET_FIELD(val, VT1_CPU_RVEC0_MSK, VT1_CPU_RVEC0_POS, rvec_addr_lo);
	rc = vt1_write(apcluster_idx,
		       CPU_MMR(pcrb_base, cpu, VT1_CPU_RVEC0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to write to RVEC1");
		return rc;
	}

	rc = vt1_write(apcluster_idx,
		       CPU_MMR(pcrb_base, cpu, VT1_CPU_NVEC0_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to write to NVEC0");
		return rc;
	}

	val = 0;
	VT1_SET_FIELD(val, VT1_CPU_RVEC1_MSK, VT1_CPU_RVEC1_POS, rvec_addr_hi);
	rc = vt1_write(apcluster_idx,
		       CPU_MMR(pcrb_base, cpu, VT1_CPU_RVEC1_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to write to RVEC1");
		return rc;
	}

	rc = vt1_write(apcluster_idx,
		       CPU_MMR(pcrb_base, cpu, VT1_CPU_NVEC1_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to write to NVEC1");
		return rc;
	}

	/* Configure PMA */
	rc = vt1_cpu_configure_pma(apcluster_idx, cpu,
				   num_pmaregion, pmaregion, pcrb_base);
	if (rc) {
		vt1_error("Failed to configure PMA for cpu - %d", cpu);
		return rc;
	}

	/* CPU RAM Init and enable for MMR access */
	rc = vt1_cpu_ram_init(apcluster_idx, cpu, pcrb_base);
	if (rc) {
		vt1_error("Failed to do RAM init for cpu - %d", cpu);
		return rc;
	}

	rc = vt1_cpu_ras_clear_state(apcluster_idx, cpu, pcrb_base);
	if (rc) {
		vt1_error("Failed to clear the CPU RAS Banks State");
		return rc;
	}

	vt1_debug("Cluster-%d: CPU init done for cpu - %d", apcluster_idx, cpu);

	return rc;
}

int vt1_cpu_reset_and_init(vmsplat_uint32_t apcluster_idx,
			   vmsplat_uint16_t cpu,
			   vmsplat_uint64_t zstage_load_addr,
			   vmsplat_uint8_t *pcl_cap,
			   vmsplat_uint32_t num_pmaregion,
			   const struct vmsplat_config_pmaregion *pmaregion,
			   vmsplat_uint64_t pcrb_base)
{
	vmsplat_uint32_t val;
	int rc = 0;
	
	if (!pcl_cap) {
		vt1_error("Invalid PCL Capability configuration\n");
		return VMSPLAT_ERR_EINVALID;
	}

	/*
	 * Reset the Hart
	 *
	 * CpuRstCtl.HardReset = 1’b1, CpuRstCtl.StartRamInit = 1’b0,
	 * CpuRstCtl.Start = 1’b0
	 */
	rc = vt1_read(apcluster_idx,
		      L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
		      &val);
	if (rc) {
		vt1_error("Failed to read to CpuRstCtl[%d]", cpu);
		return rc;
	}
	VT1_SET_FIELD(val, VT1_L3M_CPURSTCTL_HARDRESET_MSK,
			  VT1_L3M_CPURSTCTL_HARDRESET_POS,
			  1);
	VT1_SET_FIELD(val, VT1_L3M_CPURSTCTL_STARTRAMINIT_MSK,
			  VT1_L3M_CPURSTCTL_STARTRAMINIT_POS,
			  0);
	VT1_SET_FIELD(val, VT1_L3M_CPURSTCTL_START_MSK,
			  VT1_L3M_CPURSTCTL_START_POS,
			  0);
	rc = vt1_write(apcluster_idx,
		       L3M_MMR(pcrb_base, cpu, VT1_L3M_CPURSTCTL_OFFSET),
		       val);
	if (rc) {
		vt1_error("Failed to write to CpuRstCtl[%d]", cpu);
		return rc;
	}

	rc = vt1_cpu_init(apcluster_idx, cpu, zstage_load_addr,
			  pcl_cap, num_pmaregion, pmaregion, pcrb_base);
	if (rc) {
		vt1_error("Failed to initialize the cpu %d", cpu);
	}

	return rc;
}

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
					     vmsplat_uint32_t cpu))
{
	int rc;
	vmsplat_uint16_t i, snoop_val = 0;
	vmsplat_uint32_t val = 0, sys_core_ack, sys_core_req;

	if (!pcl_cap) {
		vt1_error("Invalid PCL Capability configuration\n");
		return VMSPLAT_ERR_EINVALID;
	}
	
	rc = vt1_sbr_credits_config(apcluster_idx, pcrb_base);
	if (rc) {
		vt1_error("Failed to program SBR Credits Registers");
		return rc;
	}

	if (enable_amc) {
		/* AMC Non-Core Power Estimation in SBR which is per Cluster */
		rc = CLUSTER_REG_RMW(apcluster_idx,
				     SBR_MMR(pcrb_base, VT1_SBR_POWERESTIMATIONCONTROL_OFFSET),
				     VT1_SBR_POWERESTIMATIONCONTROL_MSK,
				     VT1_SBR_POWERESTIMATIONCONTROL_POS,
				     noncore_pwr_sample_interval);
		if (rc) {
			vt1_error("Failed to program AMC PwrEstCtl");
			return rc;
		}
	}

	/* For all harts which are enabled in eFuse in a cluster */
	for (i = 0; i < num_harts; i++) {
		/* NOTE: Irrespective of the CPU config its ICS has to be 
		* programmed. One ICS is shared between 2 CPUs {n and (n+1)}
		* which will program ICS twice with same value which is alright
		* till we are programming same value in that ICS. This is 
		* happening because ICS Register Addresses are calculated 
		* per CPU */
		rc = vt1_ics_credits_config(apcluster_idx, i, pcrb_base);
		if (rc) {
			vt1_error("Failed to program ICS Credits Registers");
			return rc;
		}

		if (cpu_vector & (1 << i)) {
			vt1_debug("Initializing cpu[%d] of cluster[%d]",
					i, apcluster_idx);
			rc = vt1_cpu_init(apcluster_idx, i, zstage_load_addr,
					  pcl_cap, num_pmaregion, pmaregion,
					  pcrb_base);
			if (rc) {
				vt1_error("Failed to initialize cpu-%d\n", i);
				/* Mark cpu disabled and dont try to start later */
				if (set_cpu_disable)
					set_cpu_disable(apcluster_idx, i);
			} else {
				rc = vt1_cpu_l3m_ram_init(apcluster_idx, i,
							  pcrb_base, pcl_cap);
				if (rc) {
					vt1_error("Failed to initialize L3M for cpu %d\n", i);
					if (set_cpu_disable)
						set_cpu_disable(apcluster_idx, i);
				}

				if (enable_amc) {
					/* TODO: pwr and perf cofficients array from
					 * characterization configuation instead of NULL */
					rc = vt1_cpu_configure_amc(apcluster_idx, i,
								   pcrb_base,
								   pwr_coeffs, pwr_sample_interval,
								   perf_coeffs, perf_sample_interval,
								   noncore_perf_sample_interval);
					if (rc) {
						vt1_error("Failed to initialze AMC for cpu - %d", i);
						if (set_cpu_disable)
							set_cpu_disable(apcluster_idx, i);
					}
				}

				/* cpu-i is initialized, mark it for snoop enable */
				snoop_val |= 1U << i;

				if (set_cpu_ready)
					set_cpu_ready(apcluster_idx, i);
			}
		} else {
			/* Mark cpu disabled and dont try to start later */
			vt1_info("cpu-%d is disabled via e-Fuse", i);
			if (set_cpu_disable)
				set_cpu_disable(apcluster_idx, i);
		}
	}

	/* FIX: Not an optimial CODE above and below. Revisit Later.
	 *
	 * At this point all enabled cpus and their respective l3m slices
	 * are configured and rest are disabled in cpumask. Enable l3m snoops
	 * for each enabled cpu.
	 */

	/* For all harts which are not disabled finally after initialization */
	for (i = 0; i < num_harts; i++) {
		if (!check_cpu_disabled ||
		    !check_cpu_disabled(apcluster_idx, i)) {
			CLUSTER_REG_RMW(apcluster_idx,
					L3M_MMR(pcrb_base, i, VT1_L3M_CPUSNOOPCTL_OFFSET),
					VT1_L3M_CPUSNOOPCTL_SNOOPENABLE_MSK,
					VT1_L3M_CPUSNOOPCTL_SNOOPENABLE_POS,
					snoop_val);
		}
	}

	/* Set SystemCoherency.SysCoReq */
	rc = CLUSTER_REG_RMW(apcluster_idx,
			     SBR_MMR(pcrb_base, VT1_SBR_SYSTEMCOHERENCY_OFFSET),
			     VT1_SBR_SYSTEMCOHERENCY_SYSCOREQ_MSK,
			     VT1_SBR_SYSTEMCOHERENCY_SYSCOREQ_POS,
			     1);
	if (rc) {
		vt1_error("Failed to program SbrSysCoherency");
		return rc;
	}
	
	/* Verify SYSCOREQ assert */
	sys_core_req = 0;
	while (!sys_core_req) {
		rc = vt1_read(apcluster_idx,
			      SBR_MMR(pcrb_base, VT1_SBR_SYSTEMCOHERENCY_OFFSET),
			      &val);
		if (rc) {
			vt1_error("Failed to read SBR SystemCoherency");
			return VMSPLAT_ERR_EIO;
		}
		sys_core_req =  VT1_GET_FIELD(val,
					      VT1_SBR_SYSTEMCOHERENCY_SYSCOREQ_MSK,
					      VT1_SBR_SYSTEMCOHERENCY_SYSCOREQ_POS);
	}

	/* Poll until SYSCOACK asserts */
	val = 0;
	sys_core_ack = 0;
	while (!sys_core_ack) {
		rc = vt1_read(apcluster_idx,
			      SBR_MMR(pcrb_base, VT1_SBR_SYSTEMCOHERENCY_OFFSET),
			      &val);
		if (rc) {
			vt1_error("Failed to read SBR SystemCoherency");
			return VMSPLAT_ERR_EIO;
		}
		sys_core_ack =  VT1_GET_FIELD(val,
					      VT1_SBR_SYSTEMCOHERENCY_SYSCOACK_MSK,
					      VT1_SBR_SYSTEMCOHERENCY_SYSCOACK_POS);
	}

	return VMSPLAT_OK;
}

int vt1_cluster_start(vmsplat_uint32_t apcluster_idx,
		      vmsplat_uint32_t num_harts,
		      vmsplat_uint64_t pcrb_base,
		      vmsplat_uint16_t cpu_vector,
	void (*set_cpu_disable)(vmsplat_uint32_t apcluster_idx,
				vmsplat_uint32_t cpu),
	void (*set_cpu_online)(vmsplat_uint32_t apcluster_idx,
			       vmsplat_uint32_t cpu),
	vmsplat_bool_t (*check_cpu_disabled)(vmsplat_uint32_t apcluster_idx,
					     vmsplat_uint32_t cpu))
{
	vmsplat_uint32_t i;
	int rc;

	for (i = 0; i < num_harts; i++) {
		/*
		 * If cpu is enabled and its not already set as disabled
		 * due to previous initialization failure
		 */
		if (!(cpu_vector & (1 << i)))
			continue;
		if (check_cpu_disabled && check_cpu_disabled(apcluster_idx, i))
			continue;

		vt1_debug("starting cpu[%d] of cluster[%d]", i, apcluster_idx);
		rc = vt1_cpu_start(apcluster_idx, i, pcrb_base);
		if (rc) {
			vt1_error("Failed to start cpu-%d\n", i);
			if (set_cpu_disable)
				set_cpu_disable(apcluster_idx, i);
		} else {
			if (set_cpu_online)
				set_cpu_online(apcluster_idx, i);
		}
	}

	return VMSPLAT_OK;
}
