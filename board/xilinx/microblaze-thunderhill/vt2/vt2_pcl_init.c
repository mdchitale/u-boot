/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#define v2_fmt(fmt) "PCL: " fmt

#include <vmsplat_types.h>
#include <vt2_pcl_init.h>

static int v2_cpu_configure_pma(vmsplat_uint32_t cluster_id,
				vmsplat_uint32_t cpu,
				vmsplat_uint64_t pcrb_base,
				vmsplat_uint32_t num_pmaregion,
				const struct vmsplat_config_pmaregion *pmaregion)
{
	int rc;
	vmsplat_size_t i;
	vmsplat_uint32_t val = 0;
	vmsplat_uint32_t pma_addr_lo = 0, pma_addr_hi = 0, regaddr_pmacfg0;
	vmsplat_uint64_t pma_addr, addr, pma_addr_mask;

	if (num_pmaregion > V2_PMA_ENTRIES_MAX) {
		v2_error("PMA entries/regions provided more than supported");
		return VMSPLAT_ERR_EINVALID;
	}

	regaddr_pmacfg0 = CPU_MMR(pcrb_base, cpu, VT2_CPU_PMACFG0_OFFSET);

	for (i = 0; i < num_pmaregion; i++) {
		switch (pmaregion[i].type) {
		case VMSPLAT_PMAREGION_TYPE_WB:
			val = V2_PMA_TYPE_WB;
			rc = v2_write(cluster_id,
				      regaddr_pmacfg0 + (i * V2_PMA_REG_STRIDE),
				      val);
			if (rc) {
				v2_error("Failed to write to PMACFG[%lu]", i);
				return VMSPLAT_ERR_EIO;
			}

			break;

		case VMSPLAT_PMAREGION_TYPE_WC:
			val = V2_PMA_TYPE_WC;
			rc = v2_write(cluster_id,
				      regaddr_pmacfg0 + (i * V2_PMA_REG_STRIDE),
				      val);
			if (rc) {
				v2_error("Failed to write to PMACFG[%lu]", i);
				return VMSPLAT_ERR_EIO;
			}

			break;

		case VMSPLAT_PMAREGION_TYPE_UC:
			val = V2_PMA_TYPE_UC;
			rc = v2_write(cluster_id,
				      regaddr_pmacfg0 + (i * V2_PMA_REG_STRIDE),
				      val);
			if (rc) {
				v2_error("Failed to write to PMACFG[%lu]", i);
				return VMSPLAT_ERR_EIO;
			}
			break;

		case VMSPLAT_PMAREGION_TYPE_VACANT:
			val = V2_PMA_TYPE_VACANT;
			rc = v2_write(cluster_id,
				      regaddr_pmacfg0 + (i * V2_PMA_REG_STRIDE),
				      val);
			if (rc) {
				v2_error("Failed to write to PMACFG[%lu]", i);
				return VMSPLAT_ERR_EIO;
			}

			break;

		default:
			v2_error("Invalid PMA region type");
			return VMSPLAT_ERR_EINVALID;
		}

		/* NAPOT ranges */
		addr = pmaregion[i].addr;
		pma_addr_mask = (_ULL(1) << (pmaregion[i].order - V2_PMA_SHIFT)) - 1;
		pma_addr = ((addr >> V2_PMP_SHIFT) | pma_addr_mask);

		/* Configure PMADDRHI */
		val = 0;
		pma_addr_hi = (pma_addr >> 32) & 0xffffffff;

		V2_SET_FIELD(val, VT2_CPU_PMAADDRHI0_ADDRESS_MSK,
			     VT2_CPU_PMAADDRHI0_ADDRESS_POS,
			     pma_addr_hi);

		rc = v2_write(cluster_id,
			      CPU_MMR(pcrb_base, cpu, VT2_CPU_PMAADDRHI0_OFFSET) + (i * V2_PMA_REG_STRIDE),
			      val);
		if (rc) {
			v2_error("Failed to write to PMAADDRHI[%lu]", i);
			return VMSPLAT_ERR_EIO;
		}

		/* Configure PMADDRLO */
		val = 0;
		pma_addr_lo = pma_addr & 0xffffffff;

		V2_SET_FIELD(val, VT2_CPU_PMAADDRLO0_ADDRESS_MSK,
			     VT2_CPU_PMAADDRLO0_ADDRESS_POS,
			     pma_addr_lo);
		rc = v2_write(cluster_id,
			      CPU_MMR(pcrb_base, cpu, VT2_CPU_PMAADDRLO0_OFFSET) + (i * V2_PMA_REG_STRIDE),
			      val);
		if (rc) {
			v2_error("Failed to write to PMAADDRLO[%lu]", i);
			return VMSPLAT_ERR_EIO;
		}

	}

	v2_debug("Cluster-%d: PMA configured for cpu - %d", cluster_id, cpu);
	return VMSPLAT_OK;
}

static int v2_cpu_ras_state_clear(vmsplat_uint32_t cluster_id,
				  vmsplat_uint32_t cpu,
				  vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t regaddr_status, regaddr_addrinfo, regaddr_info, reg_stride;

	for (i = 0; i < V2_CPU_RASBANK_NUM; i++) {
		reg_stride = i * V2_CPU_RASBANK_REG_STRIDE;

		regaddr_status = CPU_MMR(pcrb_base, cpu, VT2_CPU_CPURASERRREC0STATUS_OFFSET) + reg_stride;
		/** write the lower 32-bit of register */
		rc = v2_write(cluster_id, regaddr_status, 0);
		if (rc) {
			v2_error("Clearing CPURasErrRec0StatusLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/** write the upper 32-bit of register */
		rc = v2_write(cluster_id, regaddr_status + 4, 0);
		if (rc) {
			v2_error("Clearing CPURasErrRec0StatusHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		regaddr_addrinfo = CPU_MMR(pcrb_base, cpu, VT2_CPU_CPURASERRREC0ADDRINFO_OFFSET) + reg_stride;
		/** write the lower 32-bit of register */
		rc = v2_write(cluster_id, regaddr_addrinfo, 0);
		if (rc) {
			v2_error("Clearing CPURasErrRec0AddrInfoLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/** write the upper 32-bit of register */
		rc = v2_write(cluster_id, regaddr_addrinfo + 4, 0);
		if (rc) {
			v2_error("Clearing CPURasErrRec0AddrInfoHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		regaddr_info = CPU_MMR(pcrb_base, cpu, VT2_CPU_CPURASERRREC0ADDRINFO_OFFSET) + reg_stride;
		/** write the lower 32-bit of register */
		rc = v2_write(cluster_id, regaddr_info, 0);
		if (rc) {
			v2_error("Clearing CPURasErrRec0InfoLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		/** write the upper 32-bit of register */
		rc = v2_write(cluster_id, regaddr_info + 4, 0);
		if (rc) {
			v2_error("Clearing CPURasErrRec0InfoHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

static int v2_cpu_ras_logging_signalling_enable(vmsplat_uint32_t cluster_id,
						vmsplat_uint32_t cpu,
						vmsplat_uint64_t pcrb_base,
						vmsplat_uint32_t signal_prio)
{
	int rc, i;
	vmsplat_uint32_t regaddr_rasctrl, reg_stride;
	vmsplat_uint64_t val = 0;

	if (signal_prio >= V2_RAS_SIGNAL_PRIO_MAX_IDX)
		return VMSPLAT_ERR_EINVALID;

	V2_SET_FIELD(val, VT2_CPU_CPURASERRREC0CTRL_ELSE_MSK,
		     VT2_CPU_CPURASERRREC0CTRL_ELSE_POS, 1);

	V2_SET_FIELD(val, VT2_CPU_CPURASERRREC0CTRL_CECE_MSK,
		     VT2_CPU_CPURASERRREC0CTRL_CECE_POS, 1);

	V2_SET_FIELD(val, VT2_CPU_CPURASERRREC0CTRL_CES_MSK,
		     VT2_CPU_CPURASERRREC0CTRL_CES_POS, signal_prio);

	V2_SET_FIELD(val, VT2_CPU_CPURASERRREC0CTRL_UEDS_MSK,
		     VT2_CPU_CPURASERRREC0CTRL_UEDS_POS, signal_prio);

	V2_SET_FIELD(val, VT2_CPU_CPURASERRREC0CTRL_UECS_MSK,
		     VT2_CPU_CPURASERRREC0CTRL_UECS_POS, signal_prio);

	for (i = 0; i < V2_CPU_RASBANK_NUM; i++) {
		reg_stride = i * V2_CPU_RASBANK_REG_STRIDE;
		regaddr_rasctrl = CPU_MMR(pcrb_base, cpu, VT2_CPU_CPURASERRREC0CTRL_OFFSET) + reg_stride;
		
		rc = v2_write(cluster_id, regaddr_rasctrl, (vmsplat_uint32_t)val);
		if (rc) {
			v2_error("Enable CPURasErrRec0CtrlLo Signals: CPU-%u: RasBank:-%d: %d", cpu, i, rc);
			return rc;
		}

		rc = v2_write(cluster_id, regaddr_rasctrl + 4, (vmsplat_uint32_t)(val >> 32));
		if (rc) {
			v2_error("Enable CPURasErrRec0CtrlHi Signals: CPU-%u: RasBank:-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

static int v2_l3m_ras_logging_signalling_enable(vmsplat_uint32_t cluster_id,
						vmsplat_uint32_t cpu,
						vmsplat_uint64_t pcrb_base,
						vmsplat_uint32_t signal_prio)
{
	int rc, i;
	vmsplat_uint32_t regaddr_rasctrl, reg_stride;
	vmsplat_uint64_t val = 0;

	if (signal_prio >= V2_RAS_SIGNAL_PRIO_MAX_IDX)
		return VMSPLAT_ERR_EINVALID;

	V2_SET_FIELD(val, VT2_L3M_L3MRASERRREC0CTRL_ELSE_MSK,
			VT2_L3M_L3MRASERRREC0CTRL_ELSE_POS, 1);

	V2_SET_FIELD(val, VT2_L3M_L3MRASERRREC0CTRL_CECE_MSK,
		      VT2_L3M_L3MRASERRREC0CTRL_CECE_POS, 1);

	V2_SET_FIELD(val, VT2_L3M_L3MRASERRREC0CTRL_CES_MSK,
		      VT2_L3M_L3MRASERRREC0CTRL_CES_POS, signal_prio);

	V2_SET_FIELD(val, VT2_L3M_L3MRASERRREC0CTRL_UEDS_MSK,
		      VT2_L3M_L3MRASERRREC0CTRL_UEDS_POS, signal_prio);

	V2_SET_FIELD(val, VT2_L3M_L3MRASERRREC0CTRL_UECS_MSK,
		      VT2_L3M_L3MRASERRREC0CTRL_UECS_POS, signal_prio);

	for (i = 0; i < V2_L3M_RASBANK_NUM; i++) {
		reg_stride = i * V2_L3M_RASBANK_REG_STRIDE;
		regaddr_rasctrl = L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MRASERRREC0CTRL_OFFSET) + reg_stride;

		rc = v2_write(cluster_id, regaddr_rasctrl, (vmsplat_uint32_t)val);
		if (rc) {
			v2_error("Enable L3MRasErrRec0CtrlLo Signals: CPU-%u: RasBank:-%d: %d", cpu, i, rc);
			return rc;
		}

		rc = v2_write(cluster_id, regaddr_rasctrl + 4, (vmsplat_uint32_t)(val >> 32));
		if (rc) {
			v2_error("Enable L3MRasErrRec0CtrlHi Signals: CPU-%u: RasBank:-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

static int v2_l3m_ras_state_clear(vmsplat_uint32_t cluster_id,
				  vmsplat_uint32_t cpu,
				  vmsplat_uint64_t pcrb_base)
{
	int rc, i;
	vmsplat_uint32_t regaddr_status, regaddr_addrinfo, regaddr_info;
	vmsplat_uint32_t regaddr_suppinfo, reg_stride;

	for (i = 0; i < V2_L3M_RASBANK_NUM; i++) {
		reg_stride = i * V2_L3M_RASBANK_REG_STRIDE;

		regaddr_status = L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MRASERRREC0STATUS_OFFSET) + reg_stride;
		rc = v2_write(cluster_id, regaddr_status, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0StatusLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		rc = v2_write(cluster_id, regaddr_status + 4, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0StatusHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		regaddr_addrinfo = L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MRASERRREC0ADDRINFO_OFFSET) + reg_stride;
		rc = v2_write(cluster_id, regaddr_addrinfo, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0AddrInfoLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		rc = v2_write(cluster_id, regaddr_addrinfo + 4, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0AddrInfoHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		regaddr_info = L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MRASERRREC0ADDRINFO_OFFSET) + reg_stride;
		rc = v2_write(cluster_id, regaddr_info, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0InfoLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		rc = v2_write(cluster_id, regaddr_info + 4, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0InfoHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		regaddr_suppinfo = L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MRASERRREC0SUPPLINFO_OFFSET) + reg_stride;
		rc = v2_write(cluster_id, regaddr_suppinfo, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0SupplInfoLo: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}

		rc = v2_write(cluster_id, regaddr_suppinfo + 4, 0);
		if (rc) {
			v2_error("Clearing L3MRasErrRec0SupplInfoHi: CPU-%u: RasBank-%d: %d", cpu, i, rc);
			return rc;
		}
	}

	return VMSPLAT_OK;
}

static int v2_cpu_ram_init(vmsplat_uint32_t cluster_id,
			   vmsplat_uint32_t cpu,
			   vmsplat_uint64_t pcrb_base)
{
	int rc;
	vmsplat_uint32_t val, ram_init_done = 0;

	/**
	 * 5. Management firmware writes 0b1 to StartRamInit in the
	 * CpuRstCtl register and waits until RamInitDone in the
	 * CpuRstCtl register equals 0b1.
	 */
	/* Core RAM initialization */
	rc = CLUSTER_REG_RMW(cluster_id,
			     ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
			     VT2_CPU_CPURSTCTL_STARTRAMINIT_MSK,
			     VT2_CPU_CPURSTCTL_STARTRAMINIT_POS,
			     1);
	if (rc) {
		v2_error("Set CpuRstCtl.StartRamInit: CPU-%u: %d", cpu, rc);
		return rc;
	}

	/* Poll until Core RAM Init Done */
	while (!ram_init_done) {
		v2_nsdelay(V2_RAMINIT_DELAY);

		rc = v2_read(cluster_id,
			     ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
			     &val);
		if (rc) {
			v2_error("Failed to read CpuRstCtl[%d]", cpu);
			return VMSPLAT_ERR_EIO;
		}

		ram_init_done =  V2_GET_FIELD(val,
					      VT2_CPU_CPURSTCTL_RAMINTDONE_MSK,
					      VT2_CPU_CPURSTCTL_RAMINTDONE_POS);
	}

	/**
	 * 6. Management firmware writes 0b0 to StartRamInit and 0b0
	 * to RamInitDone in the CpuRstCtl register
	 */
	rc = CLUSTER_REG_RMW(cluster_id,
			     ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
			     VT2_CPU_CPURSTCTL_STARTRAMINIT_MSK,
			     VT2_CPU_CPURSTCTL_STARTRAMINIT_POS,
			     0);
	if (rc) {
		v2_error("Set CpuRstCtl.StartRamInit: CPU-%u: %d", cpu, rc);
		return rc;
	}
	
	rc = CLUSTER_REG_RMW(cluster_id,
			     ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
			     VT2_CPU_CPURSTCTL_RAMINTDONE_MSK,
			     VT2_CPU_CPURSTCTL_RAMINTDONE_POS,
			     0);
	if (rc) {
		v2_error("Clear CpuRstCtl.StartRamInit: CPU-%u: %d", cpu, rc);
		return rc;
	}

	v2_debug("Cluster-%d: CPU RAM Init done for cpu - %d", cluster_id, cpu);
	return VMSPLAT_OK;
}

int v2_cpu_start(vmsplat_uint32_t cluster_id,
		 vmsplat_uint32_t cpu,
		 vmsplat_uint64_t pcrb_base)
{
	int rc = 0;
	
	rc = v2_cpu_ras_logging_signalling_enable(cluster_id, cpu, pcrb_base,
						  V2_RAS_SIGNAL_PRIO_LOW);
	if (rc) {
		v2_error("Failed to enable CPU RAS Signals: CPU-%u: %d", cpu, rc);
		return rc;
	}

	rc = v2_l3m_ras_logging_signalling_enable(cluster_id, cpu, pcrb_base,
						  V2_RAS_SIGNAL_PRIO_LOW);
	if (rc) {
		v2_error("Failed to enable L3M RAS Signals: CPU-%u: %d", cpu, rc);
		return rc;
	}

	/**
	 * 11. Once the SysCoAck bits in the SystemCoherency registers
	 * (for the active RN-F interfaces) equal 0b1, management
	 * firmware writes 0b1 to Start in the CpuRstCtl register
	 * for each enabled CPU.
	 */
	rc = CLUSTER_REG_RMW(cluster_id,
			     ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
			     VT2_CPU_CPURSTCTL_START_MSK,
			     VT2_CPU_CPURSTCTL_START_POS,
			     1);
	if (rc) {
		v2_error("Failed to write Start to CpuRstCtl[%d]", cpu);
	}

	return rc;
}

int v2_l3m_init(vmsplat_uint32_t cluster_id,
		vmsplat_uint32_t cpu,
		vmsplat_uint64_t pcrb_base,
		const struct v2_config_l3m_capability *l3m_uarch_config)
{
	vmsplat_uint32_t val, ram_init_done;
	int rc;
	
	if (!l3m_uarch_config) {
		v2_error("invalid parameter: l3m uarch configuration\n");
		return VMSPLAT_ERR_EINVALID;
	}

	rc = v2_write(cluster_id,
		      L3M_MMR(pcrb_base, cpu, VT2_L3M_UL3CONFIG1_OFFSET),
		      l3m_uarch_config->ul3config1);
	if (rc) {
		v2_error("failed to program Ul3Config1[%d] - %d", cpu, rc);
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_write(cluster_id,
		      L3M_MMR(pcrb_base, cpu, VT2_L3M_UL3CONFIG2_OFFSET),
		      l3m_uarch_config->ul3config2);
	if (rc) {
		v2_error("failed to program Ul3Config2[%d] - %d", cpu, rc);
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_write(cluster_id,
		      L3M_MMR(pcrb_base, cpu, VT2_L3M_UL3CONFIG3_OFFSET),
		      l3m_uarch_config->ul3config3);
	if (rc) {
		v2_error("failed to program Ul3Config3[%d] - %d", cpu, rc);
		return VMSPLAT_ERR_EIO;
	}

	rc = v2_write(cluster_id,
		      L3M_MMR(pcrb_base, cpu, VT2_L3M_UL3CONFIG4_OFFSET),
		      l3m_uarch_config->ul3config4);
	if (rc) {
		v2_error("failed to program Ul3Config4[%d] - %d", cpu, rc);
		return VMSPLAT_ERR_EIO;
	}

	/* L3M RAM Initialization */
	rc = CLUSTER_REG_RMW(cluster_id,
			     L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MCFGINIT_OFFSET),
			     VT2_L3M_L3MCFGINIT_STARTRAMINIT_MSK,
			     VT2_L3M_L3MCFGINIT_STARTRAMINIT_POS,
			     1);
	if (rc) {
		v2_error("Failed to program L3MCfgInit[%d]", cpu);
		return rc;
	}

	/* Poll until L3M RAM Init Done */
	ram_init_done = 0;
	while (!ram_init_done) {
		// TODO: Is delay here applicable, how much?
		v2_nsdelay(V2_RAMINIT_DELAY);

		rc = v2_read(cluster_id,
			     L3M_MMR(pcrb_base, cpu, VT2_L3M_L3MCFGINIT_OFFSET),
			     &val);
		if (rc) {
			v2_error("Failed to read L3MCfgInit[%d]", cpu);
			return VMSPLAT_ERR_EIO;
		}

		ram_init_done = V2_GET_FIELD(val, VT2_L3M_L3MCFGINIT_RAMINTDONE_MSK,
					     VT2_L3M_L3MCFGINIT_RAMINTDONE_POS);
	}
	
	rc = v2_l3m_ras_state_clear(cluster_id, cpu, pcrb_base);
	if (rc) {
		v2_error("Failed to clear the L3M RAS Banks State");
		return rc;
	}

	v2_debug("Cluster-%d: L3M RAM Init done for L3M-%d", cluster_id, cpu);
	return VMSPLAT_OK;
}

static int v2_cpu_capability_config(vmsplat_uint32_t cluster_id,
				    vmsplat_uint32_t cpu,
				    vmsplat_uint64_t pcrb_base,
				    const struct v2_config_cpu_isa_extension *cfg_cpu_isa,
				    const struct v2_config_cpu_capability *cfg_cpu_uarch)
{
	int rc;
	vmsplat_uint32_t val = 0;

	/**
	 * Configure CPU ISA extension configuration
	 * By default all supported extensions are enabled until disabled
	 * explicitly using the cluster library configuration.
	 */
	if (cfg_cpu_isa->disable_vector)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_VECTORDIS_MSK,
			VT2_CPU_DECISACONFIG_VECTORDIS_POS, 1);

	if (cfg_cpu_isa->disable_zvk)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZVKDIS_MSK,
			VT2_CPU_DECISACONFIG_ZVKDIS_POS, 1);

	if (cfg_cpu_isa->disable_zvfhmin)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZVFHMINDIS_MSK,
			VT2_CPU_DECISACONFIG_ZVFHMINDIS_POS, 1);

	if (cfg_cpu_isa->disable_zvfbfmin)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZVFBFMINDIS_MSK,
			VT2_CPU_DECISACONFIG_ZVFBFMINDIS_POS, 1);

	if (cfg_cpu_isa->disable_zvfbfwma)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZVFBFWMADIS_MSK,
			VT2_CPU_DECISACONFIG_ZVFBFWMADIS_POS, 1);

	if (cfg_cpu_isa->disable_xvwmatmul)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_XVWMATMULDIS_MSK,
			VT2_CPU_DECISACONFIG_XVWMATMULDIS_POS, 1);

	if (cfg_cpu_isa->disable_zfa)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZFADIS_MSK,
			VT2_CPU_DECISACONFIG_ZFADIS_POS, 1);

	if (cfg_cpu_isa->disable_zfhmin)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZFHMINDIS_MSK,
			VT2_CPU_DECISACONFIG_ZFHMINDIS_POS, 1);

	if (cfg_cpu_isa->disable_zfbfmin)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZFBFMINDIS_MSK,
			VT2_CPU_DECISACONFIG_ZFBFMINDIS_POS, 1);

	if (cfg_cpu_isa->disable_zicond)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZICONDDIS_MSK,
			VT2_CPU_DECISACONFIG_ZICONDDIS_POS, 1);

	if (cfg_cpu_isa->disable_zawrs)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZAWRSDIS_MSK,
			VT2_CPU_DECISACONFIG_ZAWRSDIS_POS, 1);

	if (cfg_cpu_isa->disable_zvkned)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_ZVKNEDDIS_MSK,
			VT2_CPU_DECISACONFIG_ZVKNEDDIS_POS, 1);

	if (cfg_cpu_isa->disable_xvwadaccu)
		V2_SET_FIELD(val, VT2_CPU_DECISACONFIG_XVWADACCUDIS_MSK,
			VT2_CPU_DECISACONFIG_XVWADACCUDIS_POS, 1);

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_DECISACONFIG_OFFSET),
			val);
	if (rc) {
		v2_error("Failed to write: DecIsaConfig[%d]", cpu);
		return rc;
	}

	/**
	 * Configure CPU uArch tweaks and chicken bits
	 */
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_PRUCONFIG1_OFFSET),
			cfg_cpu_uarch->pruconfig1);
	if (rc) {
		v2_error("Failed to write: PruConfig1[%d]", cpu);
		return VMSPLAT_ERR_EIO;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_IFUCONFIG_OFFSET),
			cfg_cpu_uarch->ifuconfig);
	if (rc) {
		v2_error("Failed to write: IfuConfig[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_DECCONFIG1_OFFSET),
			cfg_cpu_uarch->decconfig1);
	if (rc) {
		v2_error("Failed to write: DecConfig1[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_DECCONFIG2_OFFSET),
			cfg_cpu_uarch->decconfig2);
	if (rc) {
		v2_error("Failed to write: DecConfig2[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_DECLVPCONFIG_OFFSET),
			cfg_cpu_uarch->declvpconfig);
	if (rc) {
		v2_error("Failed to write: DecLvpConfig[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_GPCCONFIG_OFFSET),
			cfg_cpu_uarch->gpcconfig);
	if (rc) {
		v2_error("Failed to write: GpcConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_IXUCONFIG_OFFSET),
			cfg_cpu_uarch->ixuconfig);
	if (rc) {
		v2_error("Failed to write: IxuConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_FXUCONFIG_OFFSET),
			cfg_cpu_uarch->fxuconfig);
	if (rc) {
		v2_error("Failed to write: FxuConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_VXUCONFIG_OFFSET),
			cfg_cpu_uarch->vxuconfig);
	if (rc) {
		v2_error("Failed to write: FxuConfig[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_LSUCONFIG_OFFSET),
			cfg_cpu_uarch->lsuconfig);
	if (rc) {
		v2_error("Failed to write: LsuConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_L2MMEMUTIL_OFFSET),
			cfg_cpu_uarch->l2mmemutil);
	if (rc) {
		v2_error("Failed to write: L2mMemUtil[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_L2MCONFIG_OFFSET),
			cfg_cpu_uarch->l2mconfig);
	if (rc) {
		v2_error("Failed to write: L2mConfig[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_L2MSMPTCPCONFIG_OFFSET),
			cfg_cpu_uarch->l2msmptcpconfig);
	if (rc) {
		v2_error("Failed to write: L2mSmptcpConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_TWECONFIG_OFFSET),
			cfg_cpu_uarch->tweconfig);
	if (rc) {
		v2_error("Failed to write: TweConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_PRUCONFIG2_OFFSET),
			cfg_cpu_uarch->pruconfig2);
	if (rc) {
		v2_error("Failed to write: PruConfig2[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_PRUCONFIG3_OFFSET),
			cfg_cpu_uarch->pruconfig3);
	if (rc) {
		v2_error("Failed to write: PruConfig3[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_AFECONFIG_OFFSET),
			cfg_cpu_uarch->afeconfig);
	if (rc) {
		v2_error("Failed to write: AfeConfig[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_PRUCONFIG4_OFFSET),
			cfg_cpu_uarch->pruconfig4);
	if (rc) {
		v2_error("Failed to write: PruConfig4[%d]", cpu);
		return rc;
	}
	
	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_LSUCONFIG1_OFFSET),
			cfg_cpu_uarch->lsuconfig1);
	if (rc) {
		v2_error("Failed to write: LsuConfig1[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_CPURAMCTL_OFFSET),
			cfg_cpu_uarch->cpuramctl);
	if (rc) {
		v2_error("Failed to write: CpuRamCtl[%d]", cpu);
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_L2MCONFIG2_OFFSET),
			cfg_cpu_uarch->l2mconfig2);
	if (rc) {
		v2_error("Failed to write: L3mConfig2[%d]", cpu);
		return rc;
	}

	return VMSPLAT_OK;
}

// TODO: Not used anywhere, need to test first
/*static int v2_wait_on_register(vmsplat_uint32_t cluster_id,
			       vmsplat_uint64_t reg_addr,
			       vmsplat_uint32_t reg_field_msk,
			       vmsplat_uint32_t reg_field_pos,
			       vmsplat_uint64_t delay_in_loop)
{
	int rc;
	vmsplat_uint32_t field_done = 0, val = 0;

	while (!field_done) {
		v2_nsdelay(delay_in_loop);

		rc = v2_read(cluster_id, reg_addr, &val);
		if (rc) {
			v2_error("failed to read register address: %lx", reg_addr);
			return VMSPLAT_ERR_EIO;
		}

		field_done = V2_GET_FIELD(val, reg_field_msk, reg_field_pos);
	}
}*/

int v2_cpu_init(vmsplat_uint32_t cluster_id,
		vmsplat_uint16_t cpu,
		vmsplat_uint64_t zstage_load_addr,
		const struct v2_config_cpu_isa_extension *cfg_cpu_isa,
		const struct v2_config_cpu_capability *cfg_cpu_uarch,
		vmsplat_uint32_t num_pmaregion,
		const struct vmsplat_config_pmaregion *cfg_pmaregion,
		vmsplat_uint64_t pcrb_base)
{
	int rc = 0;
	vmsplat_uint32_t rvec_addr_lo, rvec_addr_hi, hardreset_done, val = 0;
	
	if (!cfg_cpu_isa || !cfg_cpu_uarch) {
		v2_error("invalid parameter: pcl capability configuration");
		return VMSPLAT_ERR_EINVALID;
	}

	if (!num_pmaregion || !cfg_pmaregion) {
		v2_error("invalid parameter: pma configuration");
		return VMSPLAT_ERR_EINVALID;
	}

	/* Set the CpuRstCtl.HardReset = 0 */
	rc = CLUSTER_REG_RMW(cluster_id,
			ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
			VT2_CPU_CPURSTCTL_HARDRESET_MSK,
			VT2_CPU_CPURSTCTL_HARDRESET_POS,
			0);
	if (rc) {
		v2_error("failed to program CpuRstCtl[%d]", cpu);
		return rc;
	}

	hardreset_done = 0;
	while (!hardreset_done) {
		v2_nsdelay(V2_HARDRESET_DONE_DELAY);

		rc = v2_read(cluster_id,
				ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
				&val);
		if (rc) {
			v2_error("Failed to read CpuRstCtl[%d]", cpu);
			return VMSPLAT_ERR_EIO;
		}

		hardreset_done = V2_GET_FIELD(val,
					VT2_CPU_CPURSTCTL_HARDRESETDONE_MSK,
					VT2_CPU_CPURSTCTL_HARDRESETDONE_POS);
	}

	/* Set the CpuRstCtl.HardResetDone = 0 */
	rc = CLUSTER_REG_RMW(cluster_id,
				ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
				VT2_CPU_CPURSTCTL_HARDRESETDONE_MSK,
				VT2_CPU_CPURSTCTL_HARDRESETDONE_POS,
				0);
	if (rc) {
		v2_error("Failed to program CpuRstCtl[%d]", cpu);
		return rc;
	}

	/* Enable MMR access */
	rc = CLUSTER_REG_RMW(cluster_id,
				ICC_ICP_MMR(pcrb_base, cpu, VT2_CPU_CPURSTCTL_OFFSET),
				VT2_CPU_CPURSTCTL_REGBUSENABLE_MSK,
				VT2_CPU_CPURSTCTL_REGBUSENABLE_POS,
				1);
	if (rc) {
		v2_error("Failed to set CpuRstCtl[%d].RegBusEnable", cpu);
		return rc;
	}

	rc = v2_cpu_capability_config(cluster_id, cpu, pcrb_base, cfg_cpu_isa, cfg_cpu_uarch);
	if (rc) {
		v2_error("Cluster[%d] - Failed to program cpu[%d] capabilities\n",
			  cluster_id, cpu);
		return rc;
	}

	/**
	 * 4. Management firmware configures the CPU by writing
	 * the relevant memory-mapped registers, e.g. the reset
	 * vector, the NMI vector, and the PMA registers
	 */
	/* Configure Reset and NMI Vector */
	rvec_addr_lo = (zstage_load_addr & 0xffffffff) >> 6; /* Should be 64B aligned */
	rvec_addr_hi = (zstage_load_addr >> 32) & 0xffffffff;

	val = 0;
	V2_SET_FIELD(val, VT2_CPU_RVEC0_MSK, VT2_CPU_RVEC0_POS, rvec_addr_lo);

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_RVEC0_OFFSET),
			val);
	if (rc) {
		v2_error("Failed to write to RVEC0");
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_NVEC0_OFFSET),
			val);
	if (rc) {
		v2_error("Failed to write to NVEC0");
		return rc;
	}

	val = 0;
	V2_SET_FIELD(val, VT2_CPU_RVEC1_MSK, VT2_CPU_RVEC1_POS, rvec_addr_hi);

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_RVEC1_OFFSET),
			val);
	if (rc) {
		v2_error("Failed to write to RVEC1");
		return rc;
	}

	rc = v2_write(cluster_id,
			CPU_MMR(pcrb_base, cpu, VT2_CPU_NVEC1_OFFSET),
			val);
	if (rc) {
		v2_error("Failed to write to NVEC1");
		return rc;
	}

	/* Configure PMA */
	rc = v2_cpu_configure_pma(cluster_id, cpu, pcrb_base, num_pmaregion, cfg_pmaregion);
	if (rc) {
		v2_error("Failed to configure PMA for cpu - %d", cpu);
		return rc;
	}

	/* CPU RAM Init and enable for MMR access */
	rc = v2_cpu_ram_init(cluster_id, cpu, pcrb_base);
	if (rc) {
		v2_error("Failed to do RAM init for cpu - %d", cpu);
		return rc;
	}

	rc = v2_cpu_ras_state_clear(cluster_id, cpu, pcrb_base);
	if (rc) {
		v2_error("Failed to clear the CPU RAS Banks State");
		return rc;
	}

	v2_debug("Cluster-%d: CPU init done for cpu - %d", cluster_id, cpu);

	return rc;
}

//int v2_cpu_reset_and_init(vmsplat_uint32_t cluster_id,
//			   vmsplat_uint16_t cpu,
//			   vmsplat_uint64_t zstage_load_addr,
//			   vmsplat_uint8_t *pcl_cap,
//			   vmsplat_uint32_t num_pmaregion,
//			   const struct vmsplat_config_pmaregion *pmaregion,
//			   vmsplat_uint64_t pcrb_base)
//{
//	vmsplat_uint32_t regaddr_cpurstctl, val;
//	int rc = 0;
//
//	if (!pcl_cap) {
//		v2_error("Invalid PCL Capability configuration\n");
//		return VMSPLAT_ERR_EINVALID;
//	}
//
//	regaddr_cpurstctl = V2_REG_32(pcrb_base, VT2_CPU_CPURSTCTL_OFFSET);
//	/*
//	 * Reset the Hart
//	 *
//	 * CpuRstCtl.HardReset = 1’b1, CpuRstCtl.StartRamInit = 1’b0,
//	 * CpuRstCtl.Start = 1’b0
//	 */
//	rc = v2_read(cluster_id, regaddr_cpurstctl, &val);
//	if (rc) {
//		v2_error("Failed to read to CpuRstCtl[%d]", cpu);
//		return rc;
//	}
//
//	V2_SET_FIELD(val, VT2_CPU_CPURSTCTL_HARDRESET_MSK, VT2_CPU_CPURSTCTL_HARDRESET_POS, 1);
//	V2_SET_FIELD(val, VT2_CPU_CPURSTCTL_STARTRAMINIT_MSK, VT2_CPU_CPURSTCTL_STARTRAMINIT_POS, 0);
//	V2_SET_FIELD(val, VT2_CPU_CPURSTCTL_START_MSK, VT2_CPU_CPURSTCTL_START_POS, 0);
//
//	rc = v2_write(cluster_id, regaddr_cpurstctl, val);
//	if (rc) {
//		v2_error("Failed to write to CpuRstCtl[%d]", cpu);
//		return rc;
//	}
//
//	rc = v2_cpu_init(cluster_id, cpu, zstage_load_addr, pcl_cap,
//			 num_pmaregion, pmaregion, pcrb_base);
//	if (rc) {
//		v2_error("Failed to initialize the cpu %d", cpu);
//	}
//
//	return rc;
//}

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
			struct v2_config_cpu_amc *cfg_amc)
{
	int rc;
	vmsplat_uint16_t cpu = 0, snoop_val = 0;
	vmsplat_uint32_t i, val = 0, sys_core_ack, hardreset_done;
	vmsplat_uint32_t num_rnf, cluster_impid;
		
	if (!num_harts || !cpu_vector) {
		v2_error("invalid parameter: num_harts or cpu_vector");
		return VMSPLAT_ERR_EINVALID;
	}

	if (!cfg_pclcap) {
		v2_error("invalid parameter: pcl capability configuration");
		return VMSPLAT_ERR_EINVALID;
	}

	if (enable_amc && !cfg_amc) {
		v2_error("invalid parameter: amc configuration");
		return VMSPLAT_ERR_EINVALID;
	}

	if (!num_pmaregion || !cfg_pmaregion) {
		v2_error("invalid parameter pma configuration");
		return VMSPLAT_ERR_EINVALID;
	}
	
	for (i = 0; i < num_l3m_slices; i++) {
		/**
		 * Initialize all L3M slices irrespective of whether CPU is active or not
		 * Each CPU has a L3M slice which means there is 1:1 mapping.
		 * Passing the L3M index as CPU Unit ID which is same here
		 */
		rc = v2_l3m_init(cluster_id, i, pcrb_base, &cfg_pclcap->l3m_uarch_config);
		if (rc) {
			v2_error("Failed to initialize L3M for cpu %d\n", cpu);
			return VMSPLAT_ERR_EFAIL;
		}
	}

	//TODO: Insert a minimum of 32 GCLKs delay before HardReset = 0 write
	//which happens inside the v2_cpu_init function.

	/**
	 * If the CPU is active then initialize the CPU,
	 * otherwise park the disabled CPU in sleep mode by
	 * gating the clocks and stop the init sequence
	 * for the disabled CPU.
	 */
	for (i = 0; i < num_harts; i++) {
		/** Initialize the active CPU */
		if (cpu_vector & (1 << i)) {
			rc = v2_cpu_init(cluster_id,
					 i,
					 zstage_load_addr,
					 &cfg_pclcap->cpu_isa_config,
					 &cfg_pclcap->cpu_uarch_config,
					 num_pmaregion,
					 cfg_pmaregion,
					 pcrb_base);
			if (rc) {
				v2_error("Failed to initialize active cpu-%d\n", i);
				return VMSPLAT_ERR_EFAIL;
			}

			/** Mark for snoop enable for the enabled and initialized CPUs */
			snoop_val |= 1 << i;
		}
		else { /** Park the disabled CPU in sleep */
			rc = CLUSTER_REG_RMW(cluster_id,
					     ICC_ICP_MMR(pcrb_base, i, VT2_CPU_CPURSTCTL_OFFSET),
					     VT2_CPU_CPURSTCTL_HARDRESET_MSK,
					     VT2_CPU_CPURSTCTL_HARDRESET_POS,
					     0);
			if (rc) {
				v2_error("failed to program CpuRstCtl[%d]", cpu);
				return rc;
			}

			hardreset_done = 0;
			while (!hardreset_done) {
				v2_nsdelay(V2_HARDRESET_DONE_DELAY);

				rc = v2_read(cluster_id,
					     ICC_ICP_MMR(pcrb_base, i, VT2_CPU_CPURSTCTL_OFFSET),
					     &val);
				if (rc) {
					v2_error("Failed to read CpuRstCtl[%d]", cpu);
					return VMSPLAT_ERR_EIO;
				}

				hardreset_done = V2_GET_FIELD(val,
							      VT2_CPU_CPURSTCTL_HARDRESETDONE_MSK,
							      VT2_CPU_CPURSTCTL_HARDRESETDONE_POS);
			}

			val = 0;
			V2_SET_FIELD(val, VT2_CPU_CPUCLKCTL_CLKGATEREN_MSK,
				     VT2_CPU_CPUCLKCTL_CLKGATEREN_POS, 0);
			V2_SET_FIELD(val, VT2_CPU_CPUCLKCTL_CLKGATEROVRD_MSK,
				     VT2_CPU_CPUCLKCTL_CLKGATEROVRD_POS, 0);
			V2_SET_FIELD(val, VT2_CPU_CPUCLKCTL_QUIESCEDOVRD_MSK,
				     VT2_CPU_CPUCLKCTL_QUIESCEDOVRD_POS, 0);

			rc = v2_write(cluster_id,
				      ICC_ICP_MMR(pcrb_base, i, VT2_CPU_CPURSTCTL_OFFSET),
				      val);
			if (rc) {
				v2_error("failed to write CpuRstCtl[%d]: %d", i, rc);
				return VMSPLAT_ERR_EIO;
			}
		}
	}

	/**
	 * Once all enabled CPUs have reached this point,
	 * management firmware writes 0b1 to the corresponding
	 * SnoopEnable bits for all enabled CPUs in the CpuSnpEn
	 * register in each L3M
	 */
	for (i = 0; i < num_l3m_slices; i++) {
		rc = CLUSTER_REG_RMW(cluster_id,
				     L3M_MMR(pcrb_base, i, VT2_L3M_CPUSNOOPEN_OFFSET),
				     VT2_L3M_CPUSNOOPEN_MSK,
				     VT2_L3M_CPUSNOOPEN_POS,
				     snoop_val);

		if (rc) {
			v2_error("Failed to program CpuSnoopEnable for L3M-%d\n", i);
			return VMSPLAT_ERR_EIO;
		}
	}

	
	for (i = 0; i < num_harts; i++) {
		/** Initialize the active CPU */
		if (cpu_vector & (1 << i)) {
			/**
			 * 8. Management firmware programs the CpuClkCtl register
			 * for all enabled CPUs. At a minimum, firmware writes
			 * 0b0 to ClkGaterOvrd and 0b0 to QuiescedOvrd; other
			 * fields may also be initialized
			 */
			val = 0;
			rc = v2_read(cluster_id,
				     ICC_ICP_MMR(pcrb_base, i, VT2_CPU_CPUCLKCTL_OFFSET),
				     &val);
			if (rc) {
				v2_error("Failed to read CpuClkCtl[%d]", i);
				return VMSPLAT_ERR_EIO;
			}

			V2_SET_FIELD(val, VT2_CPU_CPUCLKCTL_CLKGATEROVRD_MSK,
				     VT2_CPU_CPUCLKCTL_CLKGATEROVRD_POS, 0);
			V2_SET_FIELD(val, VT2_CPU_CPUCLKCTL_QUIESCEDOVRD_MSK,
				     VT2_CPU_CPUCLKCTL_QUIESCEDOVRD_POS, 0);

			rc = v2_write(cluster_id,
				      ICC_ICP_MMR(pcrb_base, i, VT2_CPU_CPUCLKCTL_OFFSET),
				      val);
			if (rc) {
				v2_error("Failed to write CpuClkCtl[%d]", i);
				return VMSPLAT_ERR_EIO;
			}

			/**
			 * 9. Management firmware writes 0b1 to NmiEnable in the
			 * CpuRstCtl register for all enabled CPUs
			 */
			rc = CLUSTER_REG_RMW(cluster_id,
					     ICC_ICP_MMR(pcrb_base, i, VT2_CPU_CPURSTCTL_OFFSET),
					     VT2_CPU_CPURSTCTL_NMIENABLE_MSK,
					     VT2_CPU_CPURSTCTL_NMIENABLE_POS,
					     1);
			if (rc) {
				v2_error("Set CpuRstCtl.NmiEnable: CPU-%u: %d", i, rc);
				return rc;
			}
		}
	}

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

	if (cluster_impid == V2_CLUSTER_IMPID_1)
		num_rnf = 2;
	else if (cluster_impid == V2_CLUSTER_IMPID_2)
		num_rnf = 4;
	else
		num_rnf = 1;

	for (i = 0; i < num_rnf; i++) {
		/**
		 * 10. Once the configuration of the CPUs and L3Ms is complete,
		 * management firmware writes 0b1 to SysCoReq in the
		 * SystemCoherency register for each active RN-F interface
		 * Note: At this point, the cluster must be ready to receive snoops
		 * Note: The CHI links will activate automatically upon receiving traffic
		 */
		rc = CLUSTER_REG_RMW(cluster_id,
				     MISC_MMR(pcrb_base, i, VT2_ICP_SYSTEMCOHERENCY_OFFSET),
				     VT2_ICP_SYSTEMCOHERENCY_SYSCOREQ_MSK,
				     VT2_ICP_SYSTEMCOHERENCY_SYSCOREQ_POS,
				     1);
		if (rc) {
			v2_error("Failed to program SbrSysCoherency");
			return rc;
		}
	}
	
	for (i = 0; i < num_rnf; i++) {
		/**
		 * 11. Poll to verify SysCoAck bits in the SystemCoherency
		 * registers (for the active RN-F interfaces) equal 0b1
		 */
		val = 0;
		sys_core_ack = 0;
		while (!sys_core_ack) {
			rc = v2_read(cluster_id,
				     MISC_MMR(pcrb_base, i, VT2_ICP_SYSTEMCOHERENCY_OFFSET),
				     &val);
			if (rc) {
				v2_error("Failed to read SBR SystemCoherency");
				return VMSPLAT_ERR_EIO;
			}
			sys_core_ack =  V2_GET_FIELD(val,
						     VT2_ICP_SYSTEMCOHERENCY_SYSCOACK_MSK,
						     VT2_ICP_SYSTEMCOHERENCY_SYSCOACK_POS);
		}
	}

	return VMSPLAT_OK;
}

int v2_cluster_start(vmsplat_uint32_t cluster_id,
		     vmsplat_uint32_t num_harts,
		     vmsplat_uint64_t pcrb_base,
		     vmsplat_uint16_t cpu_vector)
{
	int rc;
	vmsplat_uint32_t i;

	for (i = 0; i < num_harts; i++) {
		if (cpu_vector & (1 << i)) {
			v2_debug("starting cpu[%d] of cluster[%d]", i, cluster_id);

			rc = v2_cpu_start(cluster_id, i, pcrb_base);
			if (rc)
				v2_error("Failed to start cpu-%d of cluster-%d\n", i, cluster_id);
			else
				v2_info("started cpu[%d] of cluster[%d]", i, cluster_id);
		}
	}

	return VMSPLAT_OK;
}
