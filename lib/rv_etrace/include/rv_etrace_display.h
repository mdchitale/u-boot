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

#ifndef __RV_ETRACE_DISPLAY_H__
#define __RV_ETRACE_DISPLAY_H__

#define MAX_PARAM_LENGTH 	128
#define MAX_LINE_LENGTH		1024

int rv_etrace_pktdump(const unsigned char *packet_stream, size_t packet_stream_size,
		    size_t wp);
int rv_etrace_parse_params(const char *itrace_params[MAX_LINE_LENGTH], bool show_params);

#endif
