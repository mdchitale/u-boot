/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Macros and helper routines to access bits in a byte stream
 * based on the ratified v2.0.3 RISC-V etrace specification [1].
 *
 * [1] https://github.com/riscv-non-isa/riscv-trace-spec
 */

#include <rv_etrace_bits.h>
#include <rv_etrace_params.h>
#include <rv_etrace_payload.h>

enum rv_etrace_payload_type rv_etrace_payload_type_read(
					const struct rv_etrace_params *params,
					const struct rv_etrace_payload *payload)
{
	if (!params->packet.type_width_p)
		return RV_ETRACE_PAYLOAD_TYPE_ITRACE;

	switch (rv_etrace_read_bits(payload->data, 0, params->packet.type_width_p)) {
	case 0:
		return RV_ETRACE_PAYLOAD_TYPE_ITRACE;
	case 1:
		return RV_ETRACE_PAYLOAD_TYPE_DTRACE;
	default:
		return RV_ETRACE_PAYLOAD_TYPE_UNKNOWN;
	}
}

void rv_etrace_payload_type_write(const struct rv_etrace_params *params,
				  struct rv_etrace_payload *payload,
				  enum rv_etrace_payload_type type)
{
	if (!params->packet.type_width_p)
		return;

	switch (type) {
	case RV_ETRACE_PAYLOAD_TYPE_ITRACE:
		rv_etrace_write_bits(payload->data, 0, params->packet.type_width_p, 0);
		break;
	case RV_ETRACE_PAYLOAD_TYPE_DTRACE:
		rv_etrace_write_bits(payload->data, 0, params->packet.type_width_p, 1);
		break;
	default:
		break;
	}
}
