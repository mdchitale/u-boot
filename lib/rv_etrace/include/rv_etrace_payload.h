/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Macros and helper routines to access payload common data
 * based on the ratified v2.0.3 RISC-V etrace specification [1].
 *
 * [1] https://github.com/riscv-non-isa/riscv-trace-spec
 */

#ifndef __RV_ETRACE_PAYLOAD_H__
#define __RV_ETRACE_PAYLOAD_H__

#include <rv_etrace_params.h>

enum rv_etrace_payload_type {
	RV_ETRACE_PAYLOAD_TYPE_ITRACE = 0,
	RV_ETRACE_PAYLOAD_TYPE_DTRACE,
	RV_ETRACE_PAYLOAD_TYPE_UNKNOWN,
};

struct rv_etrace_payload {
	unsigned int size;
	unsigned char data[RV_ETRACE_PAYLOAD_MAX_BYTES];
};

enum rv_etrace_payload_type rv_etrace_payload_type_read(
					const struct rv_etrace_params *params,
					const struct rv_etrace_payload *payload);

void rv_etrace_payload_type_write(const struct rv_etrace_params *params,
				  struct rv_etrace_payload *payload,
				  enum rv_etrace_payload_type type);

#endif
