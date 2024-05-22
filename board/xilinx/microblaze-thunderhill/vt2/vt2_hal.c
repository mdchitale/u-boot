/**
 * Grovf Inc.
 * vt1 core intitialization HAL functions for simulation
 * 
 *  
*/

#include <stdio.h>
#include <stdint.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <vt2_pcl_hal.h>

#define CLUSTER_BASE  (0x80000000)
enum vmsplat_error v2_write(vmsplat_uint32_t apcluster_idx,
			     vmsplat_uint64_t reg_offset,
			     vmsplat_uint32_t reg_value)
{

	writel(reg_value, (uintptr_t)(CLUSTER_BASE + reg_offset));
	return VMSPLAT_OK;
}


enum vmsplat_error v2_read(vmsplat_uint32_t apcluster_idx,
			    vmsplat_uint64_t reg_offset,
			    vmsplat_uint32_t *reg_value)
{
	
	*reg_value = readl((uintptr_t)(CLUSTER_BASE + reg_offset));
	return VMSPLAT_OK;
}


void v2_nsdelay(vmsplat_uint64_t nsec)
{
	udelay(nsec/50);
}


int v2_printf(const char *format, ...)
{
    int res;
	va_list args;
    va_start(args, format);
    res = vprintf(format, args);
    va_end(args);
	return res;
}
