/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 */

#ifndef __V2_PCL_HAL_H__
#define __V2_PCL_HAL_H__

#include <vmsplat_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Delay function with granularity in nanoseconds
 *
 * @param[in] nsec	Nanoseconds delay
 */
void v2_nsdelay(vmsplat_uint64_t nsec);

/**
 * @brief Read PCL Register
 *
 * @param[in] cluster_id	Cluster ID (CHIPID[3:0])
 * @param[in] reg_offset	Register Offset
 * @param[in] reg_value		Buffer for Register Value
 *
 * @return enum vmsplat_error
 */
enum vmsplat_error v2_read(vmsplat_uint32_t cluster_id,
			   vmsplat_uint64_t reg_offset,
			   vmsplat_uint32_t *reg_value);

/**
 * @brief Write PCL Register
 *
 * @param[in] cluster_id	Cluster ID (CHIPID[3:0])
 * @param[in] reg_offset	Register Offset
 * @param[in] reg_value		Register Value
 *
 * @return enum vmsplat_error
 */
enum vmsplat_error v2_write(vmsplat_uint32_t cluster_id,
			    vmsplat_uint64_t reg_offset,
			    vmsplat_uint32_t reg_value);

/** @} */

#define __v2_printf(a, b) __attribute__((format(printf, a, b)))
int __v2_printf(1, 2) v2_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif


#define V2_LOG_NONE		0x00000000
#define V2_LOG_ERROR		0x00000001
#define V2_LOG_WARNING		0x00000002
#define V2_LOG_INFO		0x00000004
#define V2_LOG_DEBUG		0x00000008

#define V2_LOG_ERROR_STR	"ERROR:"
#define V2_LOG_DEBUG_STR	"DEBUG:"
#define V2_LOG_INFO_STR		"INFO:"
#define V2_LOG_WARN_STR		"WARN:"

#define V2_DEFAULT_LOG_LEVEL	(V2_LOG_INFO | \
				 V2_LOG_WARNING | \
				 V2_LOG_ERROR)

#if DEBUG
#define V2_BUILD_LOG_LEVEL	(V2_DEFAULT_LOG_LEVEL | \
				 V2_LOG_DEBUG)
#else
#define V2_BUILD_LOG_LEVEL	(V2_DEFAULT_LOG_LEVEL)
#endif

#ifndef v2_fmt
#define v2_fmt(fmt) fmt
#endif

#define v2_log(level, str, fmt, ...) \
do {	\
	if (level & V2_BUILD_LOG_LEVEL) \
		v2_printf(str " %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
} while(0)


#define v2_error(fmt, ...)		\
	v2_log(V2_LOG_ERROR, V2_LOG_ERROR_STR, v2_fmt(fmt), ##__VA_ARGS__)
#define v2_warn(fmt, ...)		\
	v2_log(V2_LOG_WARNING, V2_LOG_WARN_STR, v2_fmt(fmt), ##__VA_ARGS__)
#define v2_info(fmt, ...)		\
	v2_log(V2_LOG_INFO, V2_LOG_INFO_STR, v2_fmt(fmt), ##__VA_ARGS__)
#define v2_debug(fmt, ...)		\
	v2_log(V2_LOG_DEBUG, V2_LOG_DEBUG_STR, v2_fmt(fmt), ##__VA_ARGS__)

/**
 * V2 PCL general utility macros
 */
#define V2_REG_32(__base, __offset) 	(__base + (__offset << 2))
#define V2_REG_64(__base, __offset) 	(__base + (__offset << 3))

/* Get a bit field from a value */
#define V2_GET_FIELD(__var, __mask, __shift) \
	(((__var) & (__mask)) >> (__shift))

/* Set a bit field in a value */
#define V2_SET_FIELD(__var, __mask, __shift, __val) \
	(__var) = (((__var) & ~(__mask))  | \
		   (((__val) << (__shift)) & (__mask)))

/* Clear field by writing 0 */
#define V2_CLEAR_FIELD(__var, __mask, __shift) \
	V2_SET_FIELD(__var, __mask, __shift, 0)

#endif /* __V2_PCL_HAL_H__ */
