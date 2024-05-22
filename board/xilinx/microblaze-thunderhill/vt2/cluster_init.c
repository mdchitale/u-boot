#include <vmsplat_types.h>
#include <vt2_pcl_init.h>
#include <vt2_spx_init.h>

#define V2_CPU_RESETVALUE_PRUCONFIG1		0x24144004
#define V2_CPU_RESETVALUE_IFUCONFIG1		0x18000000
#define V2_CPU_RESETVALUE_DECISACONFIG		0x0
#define V2_CPU_RESETVALUE_DECCONFIG1		0x10000000
#define V2_CPU_RESETVALUE_DECCONFIG2		0x800020
#define V2_CPU_RESETVALUE_DECLVPCONFIG		0x1400
#define V2_CPU_RESETVALUE_GPCCONFIG		0x0
#define V2_CPU_RESETVALUE_IXUCONFIG		0x38
#define V2_CPU_RESETVALUE_FXUCONFIG		0x0
#define V2_CPU_RESETVALUE_VXUCONFIG		0x0
#define V2_CPU_RESETVALUE_LSUCONFIG		0x80000000
#define V2_CPU_RESETVALUE_L2MMEMUTIL		0x229914
#define V2_CPU_RESETVALUE_L2MCONFIG		0x20000000
#define V2_CPU_RESETVALUE_L2MSMPTCPCONFIG	0x20403
#define V2_CPU_RESETVALUE_TWECONFIG		0x0
#define V2_CPU_RESETVALUE_PRUCONFIG2		0xef8a0a8
#define V2_CPU_RESETVALUE_PRUCONFIG3		0x1cdb8ff
#define V2_CPU_RESETVALUE_AFECONFIG		0x0
#define V2_CPU_RESETVALUE_PRUCONFIG4		0x2133
#define V2_CPU_RESETVALUE_LSUCONFIG1		0x33300
#define V2_CPU_RESETVALUE_CPURAMCTL		0x9
#define V2_CPU_RESETVALUE_L2MCONFIG2		0x58e

#define V2_L3M_RESETVALUE_UL3CONFIG1		0x3
#define V2_L3M_RESETVALUE_UL3CONFIG2		0x4002f0ff
#define V2_L3M_RESETVALUE_UL3CONFIG3		0x0
#define V2_L3M_RESETVALUE_UL3CONFIG4		0x7d0fa

int cluster_init(int start)
{
	/* One Cluster with 1 Cores */
	vmsplat_uint32_t cluster_id = 0;
	vmsplat_uint32_t base_hartid = 0;
	/* Bitmap with 2 Cores enabled */
	vmsplat_uint32_t num_harts = 1;
	vmsplat_uint16_t cpu_vector = ((0x1 << num_harts) - 1);
	/* Hart(RISC-V) ==  Core */
	vmsplat_uint8_t num_l3m_slices = num_harts;
	/* TODO: VT2 - Each L3M Cache Capacity - Cache lines */
	vmsplat_uint8_t l3m_cap = 0x4;

	vmsplat_uint8_t cluster_ns = 1;

	/** cluster_node_id0 and cluster_node_id1 must not be same */
	/** TODO: These must be confirmed */
	vmsplat_uint16_t cluster_node_id0 = 0x7FF;
	vmsplat_uint16_t cluster_node_id1 = 0x7FF;

	/* Reset Vector */
	vmsplat_uint64_t zstage_load_addr = 0x400000000;
	
	/* TODO: VT2 PCL Capability and Features configuration */

	struct v2_config_pcl_capability pclcap;
	pclcap.cpu_isa_config.disable_vector = 0;
	pclcap.cpu_isa_config.disable_zvk = 0;
	pclcap.cpu_isa_config.disable_zvfhmin = 0;
	pclcap.cpu_isa_config.disable_zvfbfmin = 0;
	pclcap.cpu_isa_config.disable_zvfbfwma = 0;
	pclcap.cpu_isa_config.disable_xvwmatmul = 0;
	pclcap.cpu_isa_config.disable_zfa = 0;
	pclcap.cpu_isa_config.disable_zfhmin = 0;
	pclcap.cpu_isa_config.disable_zfbfmin = 0;
	pclcap.cpu_isa_config.disable_zicond = 0;
	pclcap.cpu_isa_config.disable_zawrs = 0;
	pclcap.cpu_isa_config.disable_zvkned = 0;
	pclcap.cpu_isa_config.disable_xvwadaccu = 0;

	/** TODO: These must be confirmed */
	pclcap.cpu_uarch_config.pruconfig1 = 0x24144004;
	pclcap.cpu_uarch_config.ifuconfig = 0x0;
	pclcap.cpu_uarch_config.decconfig1 = 0x10000000;
	pclcap.cpu_uarch_config.decconfig2 = 0x804020;
	pclcap.cpu_uarch_config.declvpconfig = 0x1400;
	pclcap.cpu_uarch_config.gpcconfig = 0x0;
	pclcap.cpu_uarch_config.ixuconfig = 0x0;
	pclcap.cpu_uarch_config.fxuconfig = 0x0;
	pclcap.cpu_uarch_config.vxuconfig = 0x10;
	pclcap.cpu_uarch_config.lsuconfig = 0x80000000;
	pclcap.cpu_uarch_config.l2mmemutil = 0x229914;
	pclcap.cpu_uarch_config.l2mconfig = 0x20080000;
	pclcap.cpu_uarch_config.l2msmptcpconfig = 0x20403;
	pclcap.cpu_uarch_config.tweconfig = 0x0;
	pclcap.cpu_uarch_config.pruconfig2 = 0xef0e0a8;
	pclcap.cpu_uarch_config.pruconfig3 = 0x1cdbeff;
	pclcap.cpu_uarch_config.afeconfig = 0xe0802;
	pclcap.cpu_uarch_config.pruconfig4 = 0x2533;
	pclcap.cpu_uarch_config.lsuconfig1 = 0x33080;
	pclcap.cpu_uarch_config.cpuramctl = 0x0;
	pclcap.cpu_uarch_config.l2mconfig2 = 0x58e;

	pclcap.l3m_uarch_config.ul3config1 = 0x0;
	pclcap.l3m_uarch_config.ul3config2 = 0x4002f0ff;
	pclcap.l3m_uarch_config.ul3config3 = 0x0;
	pclcap.l3m_uarch_config.ul3config4 = 0x7d0fa;
	
	struct v2_mmio_bases satt_base;
	satt_base.dbid_dvec_base = V2_DBMD_DVEC_BASE;
	satt_base.dbid_dreg_base = V2_DBMD_DREG_BASE;
	satt_base.dbid_puc_base = V2_DBMD_PUC_BASE;
	satt_base.lwic_mtime_base = V2_LWIC_BASE;
	satt_base.smrc_base = V2_SMRC_BASE;
	satt_base.chib_base = V2_CHIB_BASE;
	satt_base.chpl_base = V2_CHPL_BASE;
	satt_base.ckgn_base = V2_CKGN_BASE;
	satt_base.i3c_apb_base = V2_I3C_IPAPB_BASE;
	satt_base.pcl_base = V2_PCL_BASE;

	/* PMA Entries */
	vmsplat_uint32_t num_pmaregion = 11;
	struct vmsplat_config_pmaregion pmaregion[VMSPLAT_MAX_PMAREGION];
	/* Region 0 - Veyron V1 Address Space */
	pmaregion[0].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[0].addr = 0x0;
	pmaregion[0].order = 24; //16MB
	/* Region 1 - APLIC ROOT + APLIC NON-ROOT + UART 16550 */
	pmaregion[1].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[1].addr = 0x10000000;
	pmaregion[1].order = 17; //128KB
	/* Region 2 - Xilinx SDHC */
	pmaregion[2].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[2].addr = 0x10020000;
	pmaregion[2].order = 16; //64KB
	/* Region 3 - FPGA System Reset */
	pmaregion[3].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[3].addr = 0x10030000;
	pmaregion[3].order = 16; //64KB
	/* Region 4 - Xilinx AXI Ethernet */
	pmaregion[4].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[4].addr = 0x10080000;
	pmaregion[4].order = 19; //512KB
	/* Region 5 - Xilinx AXI PCIe Host0 CFG */
	pmaregion[5].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[5].addr = 0x20000000;
	pmaregion[5].order = 25; //32MB
	/* Region 6 - Xilinx AXI PCIe Host0 MMIO */
	pmaregion[6].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[6].addr = 0x40000000;
	pmaregion[6].order = 29; //512MB
	/* Region 7 - Xilinx AXI PCIe Host1 CFG */
	pmaregion[7].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[7].addr = 0x60000000;
	pmaregion[7].order = 25; //32MB
	/* Region 8 - Xilinx AXI PCIe Host1 MMIO */
	pmaregion[8].type = VMSPLAT_PMAREGION_TYPE_UC;
	pmaregion[8].addr = 0x80000000;
	pmaregion[8].order = 29; //512MB
	/* Region 9 - DRAM (Bank 1 + Bank 2)(32GB)*/
	pmaregion[9].type = VMSPLAT_PMAREGION_TYPE_WB;
	pmaregion[9].addr = 0x400000000;
	pmaregion[9].order = 35; //32GB
	/* Region 10 - VACANT */
	pmaregion[10].type = VMSPLAT_PMAREGION_TYPE_VACANT;
	pmaregion[10].addr = 0x0;
	pmaregion[10].order = 64;
	
	/** TODO: To confirm if the FPGA requires this step */
	//v2_check_bisr(cluster_id, satt_base.smrc_base);
	
	v2_config_satt(cluster_id, satt_base.smrc_base, &satt_base);

	/* 5. System De-assert */
	v2_system_deassert(cluster_id,
			    satt_base.smrc_base,
			    satt_base.chib_base,
			    satt_base.lwic_mtime_base,
			    satt_base.pcl_base,
			    base_hartid,
			    cpu_vector,
			    num_l3m_slices,
			    l3m_cap,
			    cluster_ns,
			    cluster_node_id0,
			    cluster_node_id1);

	/* 6. Cluster De-assert */
	v2_cluster_deassert(cluster_id,
			     num_harts,
			     satt_base.pcl_base,
			     cpu_vector,
			     num_l3m_slices,
			     zstage_load_addr,
			     &pclcap,
			     num_pmaregion,
			     pmaregion,
			     0,			//enable_amc = 0
			     NULL);		//cfg_amc = NULL

	/* TODO: Later for VT2 */
	//v2_dbmd_enable(0, satt_base.DBID_DVec_base, 2);


	// Start Both CPUs
	/* 7. Cluster Start */
	if (start)
		v2_cluster_start(cluster_id, num_harts, satt_base.pcl_base, cpu_vector);


//	vt2_nsdelay(2000);
//	while(1);
	//vt2_cpu_ras_dump_state(apcluster_idx, 0, satt_base.PCL_regbus_base);
	//vt2_l3m_ras_dump_state(apcluster_idx, 0, satt_base.PCL_regbus_base);

	return 0;
}
