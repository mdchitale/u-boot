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

unsigned int rv_etrace_read_bits(const unsigned char *bytes, unsigned int bit_pos,
				 unsigned int num_bits)
{
	unsigned int rd, pos, byte_index, byte_pos, byte_bits = 0;

	if (!num_bits || 32 < num_bits)
		return 0;

	for (rd = 0, pos = 0; pos < num_bits; pos += byte_bits) {
		byte_index = (bit_pos + pos) >> 3;
		byte_pos = (bit_pos + pos) & 0x7;
		byte_bits = (8 - byte_pos) < (num_bits - pos) ?
			    (8 - byte_pos) : (num_bits - pos);

		rd |= ((bytes[byte_index] >> byte_pos) & ((1U << byte_bits) - 1)) << pos;
	}

	return rd;
}

void rv_etrace_write_bits(unsigned char *bytes, unsigned int bit_pos,
			  unsigned int num_bits, unsigned int val)
{
	unsigned int pos, byte_index, byte_pos, byte_bits = 0;

	if (!num_bits || 32 < num_bits)
		return;

	for (pos = 0; pos < num_bits; pos += byte_bits) {
		byte_index = (bit_pos + pos) >> 3;
		byte_pos = (bit_pos + pos) & 0x7;
		byte_bits = (8 - byte_pos) < (num_bits - pos) ?
			    (8 - byte_pos) : (num_bits - pos);

		bytes[byte_index] &= ~(((1U << byte_bits) - 1) << byte_pos);
		bytes[byte_index] |= ((val >> pos) & ((1U << byte_bits) - 1)) << byte_pos;
	}
}

unsigned long long rv_etrace_read_bits_ll(const unsigned char *bytes, unsigned int bit_pos,
					  unsigned int num_bits)
{
	unsigned long long rd = 0;
	unsigned int pos, byte_index, byte_pos, byte_bits = 0;

	if (!num_bits || num_bits > 64)
		return 0;

	for (pos = 0; pos < num_bits; pos += byte_bits) {
		byte_index = (bit_pos + pos) >> 3;
		byte_pos = (bit_pos + pos) & 0x7;
		byte_bits = (8 - byte_pos) < (num_bits - pos) ?
			    (8 - byte_pos) : (num_bits - pos);

		rd |= ((unsigned long long)(bytes[byte_index] >> byte_pos) &
		       ((1ULL << byte_bits) - 1)) << pos;
	}

	return rd;
}

void rv_etrace_write_bits_ll(unsigned char *bytes, unsigned int bit_pos,
			     unsigned int num_bits, unsigned long long value)
{
	unsigned int pos, byte_index, byte_pos, byte_bits = 0;

	if (!num_bits || num_bits > 64)
		return;

	for (pos = 0; pos < num_bits; pos += byte_bits) {
		byte_index = (bit_pos + pos) >> 3;
		byte_pos = (bit_pos + pos) & 0x7;
		byte_bits = (8 - byte_pos) < (num_bits - pos) ?
			    (8 - byte_pos) : (num_bits - pos);

		bytes[byte_index] &= ~(((1ULL << byte_bits) - 1) << byte_pos);

		bytes[byte_index] |= ((value >> pos) & ((1ULL << byte_bits) - 1)) << byte_pos;
	}
}
