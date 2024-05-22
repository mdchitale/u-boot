/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Ventana Micro Systems Inc.
 */

#ifndef __VMSPLAT_TYPES_H__
#define __VMSPLAT_TYPES_H__

/*
 * The VMSPLAT HAL assumes that VMSPLAT_BYTES_PER_LONG define
 * will be provided as compiler flag.
 *
 * For 32-bit target, ilp32 data types are assumed which means:
 * "int"       => 32-bit
 * "long"      => 32-bit
 * "pointer"   => 32-bit
 * "long long" => 64-bit
 *
 * For 64-bit target, lp64 data types are assumed which means:
 * "int"       => 32-bit
 * "long"      => 64-bit
 * "pointer"   => 64-bit
 * "long long" => 64-bit
 */

#ifndef VMSPLAT_BYTES_PER_LONG
#define VMSPLAT_BYTES_PER_LONG __SIZEOF_LONG__
#endif

/******************************************************************************/

/**
 * \defgroup BASIC_TYPES_DEFINES Basic Data Types & Defines
 * @brief Basic data types and defines.
 * @{
 */
typedef char			vmsplat_int8_t;
typedef unsigned char		vmsplat_uint8_t;

typedef short			vmsplat_int16_t;
typedef unsigned short		vmsplat_uint16_t;

typedef int			vmsplat_int32_t;
typedef unsigned int		vmsplat_uint32_t;

#if VMSPLAT_BYTES_PER_LONG == 8
typedef long			vmsplat_int64_t;
typedef unsigned long		vmsplat_uint64_t;
#elif VMSPLAT_BYTES_PER_LONG == 4
typedef long long		vmsplat_int64_t;
typedef unsigned long long	vmsplat_uint64_t;
#else
#error "Unexpected VMSPLAT_BYTES_PER_LONG"
#endif

typedef int			vmsplat_bool_t;
typedef unsigned long		vmsplat_uintptr_t;
typedef unsigned long		vmsplat_size_t;
typedef long			vmsplat_ssize_t;

#ifndef true
#define true			1
#endif
#ifndef false
#define false			0
#endif
#ifndef NULL
#define NULL			((void *)0)
#endif
#define VMSPLAT_INVALID_ID	(~0U)

/** @brief Error Codes
 * Priority must be given to specific errors
 * instead of more general VMSPLAT_ERR_FAIL
 */
enum vmsplat_error {
	/* Success */
	VMSPLAT_OK			= 0,
	/* General fail */
	VMSPLAT_ERR_EFAIL		= -1,
	/* I/O error */
	VMSPLAT_ERR_EIO			= -2,
	/* Function not implemented */
	VMSPLAT_ERR_ENOSYS		= -3,
	/* No such device */
	VMSPLAT_ERR_ENODEV		= -4,
	/* Denied access, no perm */
	VMSPLAT_ERR_EDENIED		= -5,
	/* Invalid argument*/
	VMSPLAT_ERR_EINVALID		= -6,
	/* Operation already in progress*/
	VMSPLAT_ERR_EALREADY		= -7,
	/* Not supported*/
	VMSPLAT_ERR_ENOTSUP		= -8,
	/* No memory */
	VMSPLAT_ERR_ENOMEM		= -9,
	/* No such entry/id */
	VMSPLAT_ERR_ENOENT		= -10,
};

/**
 * \ingroup VMSPLAT_CONFIG
 * @brief Maximum number of PMA regions supported for each CPU
 */
#define VMSPLAT_MAX_PMAREGION		64

/**
 * \ingroup VMSPLAT_CONFIG
 * @brief Types of one PMA regions
 */
enum vmsplat_config_pmaregion_type {
	VMSPLAT_PMAREGION_TYPE_WB = 0,
	VMSPLAT_PMAREGION_TYPE_WC,
	VMSPLAT_PMAREGION_TYPE_UC,
	VMSPLAT_PMAREGION_TYPE_VACANT,
};

/**
 * \ingroup VMSPLAT_CONFIG
 * @brief Physical Memory Attributes(PMA) details
 * of one memory region
 */
struct vmsplat_config_pmaregion {
	enum vmsplat_config_pmaregion_type type;
	vmsplat_uint64_t addr;
	vmsplat_uint32_t order;
};

/** @} */

#endif  /* __VMSPLAT_TYPES_H__ */
