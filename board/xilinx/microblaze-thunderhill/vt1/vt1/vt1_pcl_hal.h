/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#ifndef __VT1_PCL_HAL_H__
#define __VT1_PCL_HAL_H__

#include <vmsplat_types.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * \defgroup VT1_PCL_HAL VT1 PCL HAL Interface
 * @brief Functions which are used by PCL SW IP
 * and to be implemented by the PCL SW IP Host
 * firmware
 * @{
 */

/**
 * @brief Delay function with granularity in nanoseconds
 *
 * @param[in] nsec	Nanoseconds delay
 */
void vt1_nsdelay(vmsplat_uint64_t nsec);


/**
 * @brief Read PCL Register
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] reg_offset	Register Offset
 * @param[in] reg_value		Buffer for Register Value
 *
 * @return enum vmsplat_error
 */
enum vmsplat_error vt1_read(vmsplat_uint32_t apcluster_idx,
			    vmsplat_uint64_t reg_offset,
			    vmsplat_uint32_t *reg_value);

/**
 * @brief Write PCL Register
 *
 * @param[in] apcluster_idx	Cluster ID
 * @param[in] reg_offset	Register Offset
 * @param[in] reg_value		Register Value
 *
 * @return enum vmsplat_error
 */
enum vmsplat_error vt1_write(vmsplat_uint32_t apcluster_idx,
			     vmsplat_uint64_t reg_offset,
			     vmsplat_uint32_t reg_value);

/** @} */

#define __vt1_printf(a, b) __attribute__((format(printf, a, b)))
int __vt1_printf(1, 2) vt1_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif


#define VT1_LOG_NONE		0x00000000
#define VT1_LOG_ERROR		0x00000001
#define VT1_LOG_WARNING		0x00000002
#define VT1_LOG_INFO		0x00000004
#define VT1_LOG_DEBUG		0x00000008

#define VT1_LOG_ERROR_STR	"ERROR:"
#define VT1_LOG_DEBUG_STR	"DEBUG:"
#define VT1_LOG_INFO_STR	"INFO:"
#define VT1_LOG_WARN_STR	"WARN:"

#define VT1_DEFAULT_LOG_LEVEL	(VT1_LOG_INFO | \
				 VT1_LOG_WARNING | \
				 VT1_LOG_ERROR )

#ifdef DEBUG
#define VT1_BUILD_LOG_LEVEL	(VT1_DEFAULT_LOG_LEVEL | \
				 VT1_LOG_DEBUG)
#else
#define VT1_BUILD_LOG_LEVEL	(VT1_DEFAULT_LOG_LEVEL)
#endif

#ifndef vt1_fmt
#define vt1_fmt(fmt) fmt
#endif

#define vt1_log(level, str, fmt, ...) \
do {	\
	if (level & VT1_BUILD_LOG_LEVEL) \
		vt1_printf(str " %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
} while(0)


#define vt1_error(fmt, ...)		\
	vt1_log(VT1_LOG_ERROR, VT1_LOG_ERROR_STR, vt1_fmt(fmt), ##__VA_ARGS__)
#define vt1_warn(fmt, ...)		\
	vt1_log(VT1_LOG_WARNING, VT1_LOG_WARN_STR, vt1_fmt(fmt), ##__VA_ARGS__)
#define vt1_info(fmt, ...)		\
	vt1_log(VT1_LOG_INFO, VT1_LOG_INFO_STR, vt1_fmt(fmt), ##__VA_ARGS__)
#define vt1_debug(fmt, ...)		\
	vt1_log(VT1_LOG_DEBUG, VT1_LOG_DEBUG_STR, vt1_fmt(fmt), ##__VA_ARGS__)

/**
 * VT1 PCL general utility macros
 */
#define VT1_REG_32(__base, __offset) 	(__base + (__offset * 4))
#define VT1_REG_64(__base, __offset) 	(__base + (__offset * 8))

/* Get a bit field from a value */
#define VT1_GET_FIELD(__var, __mask, __shift) \
	(((__var) & (__mask)) >> (__shift))

/* Set a bit field in a value */
#define VT1_SET_FIELD(__var, __mask, __shift, __val) \
	(__var) = (((__var) & ~(__mask))  | \
		   (((__val) << (__shift)) & (__mask)))

/* Clear field by writing 0 */
#define VT1_CLEAR_FIELD(__var, __mask, __shift) \
	VT1_SET_FIELD(__var, __mask, __shift, 0)


#endif /* __VT1_PCL_HAL_H__ */
