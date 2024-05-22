
#include <vt1/vt1_pcl_init.h>
#include <vt1/vt1_spx_init.h>
#include <stdlib.h>
int cluster_init( int start)
{
	/* One Cluster with 2 Cores */
	vmsplat_uint32_t apcluster_idx = 0;
	vmsplat_uint32_t base_hartid = 0;
	/* Bitmap with 2 Cores enabled */
	vmsplat_uint32_t num_harts = 2;
	vmsplat_uint16_t cpu_vector = ((0x1 << num_harts) - 1);
	/* Hart(RISC-V) ==  Core */
	vmsplat_uint8_t num_ul3_slices = num_harts - 1;
	/* Each L3M Cache Capacity - Cache lines */
	vmsplat_uint8_t ul3_cap = 0x4;
	/* Reset Vector */
	vmsplat_uint64_t zstage_load_addr = 0x400000000;
	/* PCL Capability and Features configuration */
	vmsplat_uint8_t pcl_cap[VT1_PCL_CAP_CONFIG_ARRAY_MAX];
	pcl_cap[VT1_PCL_CAP_AIA] = VT1_PCL_CAP_ENABLE;
	pcl_cap[VT1_PCL_CAP_SVINVAL] = VT1_PCL_CAP_ENABLE;
	pcl_cap[VT1_PCL_CAP_CBO] = VT1_PCL_CAP_ENABLE;
	pcl_cap[VT1_PCL_CAP_ZABCS] = VT1_PCL_CAP_ENABLE;
	pcl_cap[VT1_PCL_CAP_VSCONDOPS] = VT1_PCL_CAP_ENABLE;
	pcl_cap[VT1_PCL_CAP_CACHE] = VT1_PCL_CAP_ENABLE;
	pcl_cap[VT1_PCL_CAP_SNOOP] = VT1_PCL_CAP_ENABLE;

	/* SATT Base addresses */
	struct vt1_mmio_bases satt_base;
	satt_base.DBID_DVec_base = 0x0;
	satt_base.DBID_DReg_base = 0x1000;
	satt_base.DBID_PuC_base = 0x2000;
	satt_base.LWIC_Mtime_base = 0x4000;
	satt_base.SMRC_base = 0x8000;
	satt_base.CHIB_base = 0xC000;
	satt_base.SDDI_base = 0x20000;
	satt_base.CKGN_base = 0x30000;
	satt_base.I3C_APB_base = 0x3F000;
	satt_base.PCL_regbus_base = 0x800000;

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

	/* 3. Configure SATT */
	vt1_config_satt(apcluster_idx, satt_base.SMRC_base, &satt_base);

	/* 4. Enable MTIME */
	vt1_enable_mtime(apcluster_idx, satt_base.LWIC_Mtime_base);

	/* 5. System De-assert */
	vt1_system_deassert(apcluster_idx,
			satt_base.SMRC_base,
			base_hartid,
			cpu_vector,
			num_ul3_slices,
			ul3_cap);

	/* 6. Cluster De-assert */
	vt1_cluster_deassert(apcluster_idx,
			num_harts,
			satt_base.PCL_regbus_base,
			cpu_vector,
			zstage_load_addr,
			pcl_cap,
			num_pmaregion,
			pmaregion,
			0,
			NULL,
			0,
			NULL,
			0,
			0,
			0,
			NULL,
			NULL,
			NULL);

	vt1_dbmd_enable(0, satt_base.DBID_DVec_base, 2);


	// Start Both CPUs
	/* 7. Cluster Start */
	if (start) vt1_cluster_start(apcluster_idx,
			2,
			satt_base.PCL_regbus_base,
			cpu_vector,
			NULL,
			NULL,
			NULL);


	//	vt1_nsdelay(2000);
	//	while(1);
	//vt1_cpu_ras_dump_state(apcluster_idx, 0, satt_base.PCL_regbus_base);
	//vt1_l3m_ras_dump_state(apcluster_idx, 0, satt_base.PCL_regbus_base);

	return 0;
}
