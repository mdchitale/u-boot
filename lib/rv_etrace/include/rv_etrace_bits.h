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

#ifndef __RV_ETRACE_BITS_H__
#define __RV_ETRACE_BITS_H__

static inline unsigned int rv_etrace_bits_to_bytes(unsigned int num_bits)
{
	return (num_bits + 7) >> 3;
}

unsigned int rv_etrace_read_bits(const unsigned char *bytes, unsigned int bit_pos,
				 unsigned int num_bits);

void rv_etrace_write_bits(unsigned char *bytes, unsigned int bit_pos,
			  unsigned int num_bits, unsigned int val);

unsigned long long rv_etrace_read_bits_ll(const unsigned char *bytes, unsigned int bit_pos,
					  unsigned int num_bits);

void rv_etrace_write_bits_ll(unsigned char *bytes, unsigned int bit_pos,
			     unsigned int num_bits, unsigned long long value);

#endif
