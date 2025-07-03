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
#include <stdio.h>
#include <string.h>
#include <rv_etrace_bits.h>
#include <rv_etrace_params.h>
#include <rv_etrace_itrace.h>


/** Function to calculate irdepth based on return_stack_size_p and call_counter_size_p */
static inline unsigned int calculate_irdepth(unsigned int return_stack_size_p,
					     unsigned int call_counter_size_p)
{
	return return_stack_size_p + (return_stack_size_p > 0 ? 1 : 0) + call_counter_size_p;
}

static int rv_branch_map_valid_bits(unsigned int branches)
{
	/** Define the branch limits in terms of valid bits */
	if (branches == 0)
		return 0;
	else if (branches == 1)
		return 1;
	else if (branches <= 3)
		return 3;
	else if (branches <= 7)
		return 7;
	else if (branches <= 15)
		return 15;
	else if (branches <= 31)
		return 31;
	else /* For cases where the branch number exceeds 31 */
		return 31;
}

static void rv_itrace_pkt_decomp(const struct rv_etrace_payload *payload)
{
	unsigned int msb, i;

	msb = rv_etrace_read_bits(payload->data, payload->size * 8 - 1, 1);
	for (i = payload->size; i < RV_ETRACE_PAYLOAD_MAX_BYTES; i++)
		if (msb)
			memset((void *)&payload->data[i], 0xff, 1);
		else
			memset((void *)&payload->data[i], 0x0, 1);
}

static void rv_itrace_read_iaddress(const struct rv_etrace_params *params,
					    const struct rv_etrace_payload *payload,
					    struct rv_itrace_iaddress *iaddr,
					    unsigned int bit_pos)
{
	unsigned int bit_len, payload_bits = payload->size * 8;
	unsigned int addr_msb;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	if (bit_pos + bit_len >= payload_bits)
		rv_itrace_pkt_decomp(payload);

	iaddr->addr = rv_etrace_read_bits_ll(payload->data, bit_pos, bit_len);
	addr_msb = iaddr->addr >> (bit_len - 1);
	bit_pos += bit_len;
	iaddr->addr = iaddr->addr << params->itrace.iaddress_lsb_p;

	if (bit_pos + 1 < payload_bits) {
		iaddr->notify = rv_etrace_read_bits(payload->data, bit_pos, 1) ^ addr_msb;
		bit_pos += 1;
	} else
		iaddr->notify = 0;

	if (bit_pos + 1 < payload_bits) {
		iaddr->updiscon = rv_etrace_read_bits(payload->data, bit_pos, 1) ^ addr_msb;
		bit_pos += 1;
	} else
		iaddr->updiscon = 0;

	if (bit_pos + 1 < payload_bits) {
		iaddr->irreport = rv_etrace_read_bits(payload->data, bit_pos, 1) ^ addr_msb;
		bit_pos += 1;
	} else
		iaddr->irreport = 0;
	
	if (iaddr->irreport) {
		bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				       	    params->itrace.call_counter_size_p);
		iaddr->irdepth = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
		bit_pos += bit_len;
	}
}

/** Function to get number of bits required by itrace format=0  subformat =0  */
static unsigned int rv_itrace_format00_bits(const struct rv_etrace_params *params,
						const struct rv_itrace_data *it)
{
	unsigned int ret = 32;
	ret += 2;
	if (it->format0.format00.branch_fmt == 0)
		return ret;
	ret += params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	ret += 3;
	ret += calculate_irdepth(params->itrace.return_stack_size_p,
				 params->itrace.call_counter_size_p);
	return ret;
}

/** Function to decode itrace format=0  subformat =0  from payload */
static int rv_itrace_format00_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format00 *fmt0)
{
	unsigned int bit_len = 32, payload_bits = payload->size * 8;

	if (bit_pos + bit_len >= payload_bits)
		bit_len = payload_bits - bit_pos;
	fmt0->branch_count = rv_etrace_read_bits(payload->data, bit_pos,
					       bit_len);
	bit_pos += bit_len;
	bit_len =  2;
	if (bit_pos + bit_len >= payload_bits)
		fmt0->branch_fmt = 0;
	else
		fmt0->branch_fmt = rv_etrace_read_bits(payload->data, bit_pos,
						       bit_len);
	if (fmt0->branch_fmt == 0)
		return 0;

	bit_pos += bit_len;
	rv_itrace_read_iaddress(params, payload, &fmt0->iaddress, bit_pos);

	return 0;
}

/** Function to encode itrace format=0  subformat =0  into payload */
static int rv_itrace_format00_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format00 *fmt0)
{
	unsigned int bit_len = 32;
	rv_etrace_write_bits(payload->data, bit_pos,
			     bit_len,
			     fmt0->branch_count);
	bit_pos += bit_len;
	bit_len = 2;
	if (fmt0->branch_fmt == 0)
		return 0;
	rv_etrace_write_bits(payload->data, bit_pos,
			     bit_len,
			     fmt0->branch_fmt);
	bit_pos += bit_len;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	rv_etrace_write_bits_ll(payload->data, bit_pos, bit_len,
				fmt0->iaddress.addr >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt0->iaddress.notify);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt0->iaddress.updiscon);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt0->iaddress.irreport);
	bit_pos += 1;
	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt0->iaddress.irdepth);
	bit_pos += bit_len;
	return 0;
}

/** Function to get number of bits required by itrace format=0 subformat = 1  */
static unsigned int rv_itrace_format01_bits(const struct rv_etrace_params *params,
						const struct rv_itrace_data *it)
{
	unsigned int ret = params->itrace.cache_size_p ;
	ret += 5;
	ret += rv_branch_map_valid_bits(it->format0.format01.branches);
	if (it->format0.format01.branches == 31)
		return ret;
	ret += 1;
	ret += calculate_irdepth(params->itrace.return_stack_size_p,
				params->itrace.call_counter_size_p);
	return ret;
}

/** Function to decode itrace fformat=0 subformat = 1 from payload */
static int rv_itrace_format01_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format01 *fmt01)
{
	unsigned int bit_len = params->itrace.cache_size_p, prev_msb;
	unsigned int payload_bits = payload->size * 8;

	fmt01->index = rv_etrace_read_bits(payload->data, bit_pos,
					       bit_len);
	bit_pos += bit_len;
	bit_len = 5;
	fmt01->branches = rv_etrace_read_bits(payload->data, bit_pos,
					       bit_len);
	bit_pos += bit_len;
	bit_len = rv_branch_map_valid_bits(fmt01->branches);
	if (bit_len) {
		if (bit_pos + bit_len >= payload_bits)
			rv_itrace_pkt_decomp(payload);
		fmt01->branch_map = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
		bit_pos += bit_len;
		prev_msb = fmt01->branch_map >> (bit_len - 1);
	} else
		prev_msb = fmt01->branches >> 4;

	if (bit_pos + 1 < payload_bits) {
		fmt01->irreport = rv_etrace_read_bits(payload->data, bit_pos, 1);
		bit_pos += 1;
	} else
		fmt01->irreport = prev_msb;
	
	fmt01->irreport = prev_msb ^ fmt01->irreport;
	if (fmt01->irreport) {
		bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
					    params->itrace.call_counter_size_p);
		fmt01->irdepth = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
		bit_pos += bit_len;
	}
	return 0;
}

/** Function to encode itrace format0 into payload */
static int rv_itrace_format01_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format01 *fmt01)
{
	unsigned int bit_len = params->itrace.cache_size_p ;

	rv_etrace_write_bits(payload->data, bit_pos,
			     bit_len,
			     fmt01->index);
	bit_pos += bit_len;
	bit_len = 5;
	rv_etrace_write_bits(payload->data, bit_pos,
			     bit_len,
			     fmt01->branches);
	bit_pos += bit_len;
	bit_len = rv_branch_map_valid_bits(fmt01->branches);
	rv_etrace_write_bits(payload->data, bit_pos,
			     bit_len,
			     fmt01->branch_map);
	bit_pos += bit_len;
	if (fmt01->branches == 31)
		return 0;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt01->irreport);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt01->irdepth);
	bit_pos += bit_len;
	return 0;
}

/** Function to get number of bits required by itrace format=1  */
static unsigned int rv_itrace_format1_bits(const struct rv_etrace_params *params,
					   const struct rv_itrace_data *it)
{
	unsigned int ret = 5;

	ret += rv_branch_map_valid_bits(it->format1.branches);
	if (it->format1.branches == 31)
		return ret;

	ret += params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	ret += 3;
	ret += calculate_irdepth(params->itrace.return_stack_size_p,
				 params->itrace.call_counter_size_p);
	return ret;
}

/** Function to decode itrace format=1 from payload */
static int rv_itrace_format1_read(const struct rv_etrace_params *params,
				  const struct rv_etrace_payload *payload,
				  unsigned int bit_pos,
				  struct rv_itrace_format1 *fmt1)
{
	unsigned int bit_len = 5;

	fmt1->branches = rv_etrace_read_bits(payload->data, bit_pos, 5);
	bit_pos += 5;

	bit_len = rv_branch_map_valid_bits(fmt1->branches);
	if (!bit_len)
		bit_len = 31;

	fmt1->branch_map = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
	if (bit_len == 31)
		return 0;

	bit_pos += bit_len;
	rv_itrace_read_iaddress(params, payload, &fmt1->iaddress, bit_pos);

	return 0;
}

/** Function to encode itrace format=1 into payload */
static int rv_itrace_format1_write(const struct rv_etrace_params *params,
				   struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   const struct rv_itrace_format1 *fmt1)
{
	unsigned int bit_len = 5;

	rv_etrace_write_bits(payload->data, bit_pos,
			     5, fmt1->branches);
	bit_pos += 5;

	bit_len = rv_branch_map_valid_bits(fmt1->branches);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt1->branch_map);
	bit_pos += bit_len;

	if (fmt1->branches == 31)
		return 0;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	rv_etrace_write_bits_ll(payload->data, bit_pos, bit_len,
				fmt1->iaddress.addr >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt1->iaddress.notify);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt1->iaddress.updiscon);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt1->iaddress.irreport);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt1->iaddress.irdepth);
	bit_pos += bit_len;
	return 0;
}

/** Function to get number of bits required by itrace format2 */
static unsigned int rv_itrace_format2_bits(const struct rv_etrace_params *params)
{
	unsigned int ret = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;

	ret += 3;
	ret += calculate_irdepth(params->itrace.return_stack_size_p,
				 params->itrace.call_counter_size_p);

	return ret;
}

/** Function to decode itrace format2 from payload */
static int rv_itrace_format2_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format2 *fmt2)
{
	rv_itrace_read_iaddress(params, payload, &fmt2->iaddress, bit_pos);
	return 0;
}

/** Function to encode itrace format2 into payload */
static int rv_itrace_format2_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format2 *fmt2)
{
	unsigned int bit_len = 0;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	rv_etrace_write_bits_ll(payload->data, bit_pos, bit_len,
				fmt2->iaddress.addr >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt2->iaddress.notify);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt2->iaddress.updiscon);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt2->iaddress.irreport);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt2->iaddress.irdepth);
	bit_pos += bit_len;

	return 0;
}

/** Function to get number of bits required by itrace format=3 sub-format=3 */
static unsigned int rv_itrace_format33_bits(const struct rv_etrace_params *params)
{
	unsigned int ret = 1; /* ienable */
	ret += 2; /* qual_stat */
	ret += 4; /* ioptions */
	return ret;
}

/** Function to decode itrace format33 from payload */
static int rv_itrace_format33_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format33 *fmt33)
{
	fmt33->ienable = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	fmt33->qual_status = rv_etrace_read_bits(payload->data, bit_pos, 2);
	bit_pos += 2;

	fmt33->ioptions = rv_etrace_read_bits(payload->data, bit_pos, 32);
	bit_pos += 4;

	return 0;
}

/** Function to encode itrace format33 into payload */
static int rv_itrace_format33_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format33 *fmt33)
{
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt33->ienable);
	bit_pos += 1;

	rv_etrace_write_bits(payload->data, bit_pos, 32, fmt33->encoder_mode);
	bit_pos += 32;

	rv_etrace_write_bits(payload->data, bit_pos, 2, fmt33->qual_status);
	bit_pos += 2;

	rv_etrace_write_bits(payload->data, bit_pos, 32, fmt33->ioptions);
	bit_pos += 32;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt33->denable);
	bit_pos += 1;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt33->dloss);
	bit_pos += 1;

	rv_etrace_write_bits(payload->data, bit_pos, 32, fmt33->doptions);
	bit_pos += 32;

	return 0;
}

/** Function to get number of bits required by itrace format32 */
static unsigned int rv_itrace_format32_bits(const struct rv_etrace_params *params)
{
	unsigned int ret = params->itrace.privilege_width_p;

	if (!params->itrace.notime_p)
		ret += params->itrace.time_width_p;

	if (!params->itrace.nocontext_p)
		ret += params->itrace.context_width_p;

	return ret;
}

/** Function to decode itrace format32 from payload */
static int rv_itrace_format32_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format32 *fmt32)
{
	fmt32->privilege = rv_etrace_read_bits(payload->data, bit_pos,
					       params->itrace.privilege_width_p);
	bit_pos += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p) {
		fmt32->time = rv_etrace_read_bits(payload->data, bit_pos,
						  params->itrace.time_width_p);
		bit_pos += params->itrace.time_width_p;
	}

	if (!params->itrace.nocontext_p) {
		fmt32->context = rv_etrace_read_bits(payload->data, bit_pos,
						     params->itrace.context_width_p);
		bit_pos += params->itrace.context_width_p;
	}

	return 0;
}

/** Function to encode itrace format32 into payload */
static int rv_itrace_format32_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format32 *fmt32)
{
	rv_etrace_write_bits(payload->data, bit_pos,
			     params->itrace.privilege_width_p,
			     fmt32->privilege);
	bit_pos += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p) {
		rv_etrace_write_bits(payload->data, bit_pos,
				     params->itrace.time_width_p,
				     fmt32->time);
		bit_pos += params->itrace.time_width_p;
	}

	if (!params->itrace.nocontext_p) {
		rv_etrace_write_bits(payload->data, bit_pos,
				     params->itrace.context_width_p,
				     fmt32->context);
		bit_pos += params->itrace.context_width_p;
	}

	return 0;
}

/** Function to get number of bits required by itrace format31 */
static unsigned int rv_itrace_format31_bits(const struct rv_etrace_params *params,
					    const struct rv_itrace_format31 *fmt31)
{
	unsigned int ret = 1;

	ret += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p)
		ret += params->itrace.time_width_p;

	if (!params->itrace.nocontext_p)
		ret += params->itrace.context_width_p;

	ret += params->itrace.ecause_width_p;

	ret += 2;
	ret += params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	ret += params->itrace.iaddress_width_p;
	return ret;
}

/** Function to decode itrace format31 from payload */
static int rv_itrace_format31_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format31 *fmt31)
{
	unsigned int bit_len = 0;

	fmt31->branch = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	fmt31->privilege = rv_etrace_read_bits(payload->data, bit_pos,
					       params->itrace.privilege_width_p);
	bit_pos += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p) {
		fmt31->time = rv_etrace_read_bits(payload->data, bit_pos,
						  params->itrace.time_width_p);
		bit_pos += params->itrace.time_width_p;
	}

	if (!params->itrace.nocontext_p) {
		fmt31->context = rv_etrace_read_bits(payload->data, bit_pos,
						     params->itrace.context_width_p);
		bit_pos += params->itrace.context_width_p;
	}

	fmt31->ecause = rv_etrace_read_bits(payload->data, bit_pos,
					    params->itrace.ecause_width_p);
	bit_pos += params->itrace.ecause_width_p;

	fmt31->interrupt = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	fmt31->thaddr = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	fmt31->address = rv_etrace_read_bits_ll(payload->data, bit_pos, bit_len);
	fmt31->address = fmt31->address << params->itrace.iaddress_lsb_p;
	bit_pos += bit_len;

	if (!fmt31->interrupt) {
		fmt31->tval = rv_etrace_read_bits_ll(payload->data, bit_pos,
						     params->itrace.iaddress_width_p);
		bit_pos += params->itrace.iaddress_width_p;
	}
	return 0;
}

/** Function to encode itrace format31 into payload */
static int rv_itrace_format31_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format31 * fmt31)
{
	unsigned int bit_len = 0;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt31->branch);
	bit_pos += 1;

	rv_etrace_write_bits(payload->data, bit_pos, params->itrace.privilege_width_p,
			     fmt31->privilege);
	bit_pos += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p) {
		rv_etrace_write_bits(payload->data, bit_pos, params->itrace.time_width_p,
				     fmt31->time);
		bit_pos += params->itrace.time_width_p;
	}

	if (!params->itrace.nocontext_p) {
		rv_etrace_write_bits(payload->data, bit_pos, params->itrace.context_width_p,
				     fmt31->context);
		bit_pos += params->itrace.context_width_p;
	}

	rv_etrace_write_bits(payload->data, bit_pos, params->itrace.ecause_width_p,
			     fmt31->ecause);
	bit_pos += params->itrace.ecause_width_p;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt31->interrupt);
	bit_pos += 1;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt31->thaddr);
	bit_pos += 1;

	bit_len =params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	rv_etrace_write_bits_ll(payload->data, bit_pos, bit_len,
				fmt31->address >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;

	if (!fmt31->interrupt) {
		rv_etrace_write_bits_ll(payload->data, bit_pos,
					params->itrace.iaddress_width_p, fmt31->tval);
		bit_pos += params->itrace.iaddress_width_p;
	}
	return 0;
}

/** Function to get number of bits required by itrace format30 */
static unsigned int rv_itrace_format30_bits(const struct rv_etrace_params *params)
{
	unsigned int ret = 1;

	ret += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p)
		ret += params->itrace.time_width_p;

	if (!params->itrace.nocontext_p)
		ret += params->itrace.context_width_p;

	ret += params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;

	return ret;
}

/** Function to decode itrace format30 from payload */
static int rv_itrace_format30_read(const struct rv_etrace_params *params,
				   const struct rv_etrace_payload *payload,
				   unsigned int bit_pos,
				   struct rv_itrace_format30 *fmt30)
{
	unsigned long bit_len = 0, payload_bits = payload->size * 8;

	fmt30->branch = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	fmt30->privilege = rv_etrace_read_bits(payload->data, bit_pos,
					       params->itrace.privilege_width_p);
	bit_pos += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p) {
		fmt30->time = rv_etrace_read_bits(payload->data, bit_pos,
						  params->itrace.time_width_p);
		bit_pos += params->itrace.time_width_p;
	}

	if (!params->itrace.nocontext_p) {
		fmt30->context = rv_etrace_read_bits(payload->data, bit_pos,
						     params->itrace.context_width_p);
		bit_pos += params->itrace.context_width_p;
	}

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	if (bit_pos + bit_len >= payload_bits)
		rv_itrace_pkt_decomp(payload);

	fmt30->address = rv_etrace_read_bits_ll(payload->data, bit_pos, bit_len);
	fmt30->address = fmt30->address << params->itrace.iaddress_lsb_p;
	bit_pos += bit_len;

	return 0;
}

/** Function to encode itrace format30 into payload */
static int rv_itrace_format30_write(const struct rv_etrace_params *params,
				    struct rv_etrace_payload *payload,
				    unsigned int bit_pos,
				    const struct rv_itrace_format30 *fmt30)
{
	unsigned long bit_len = 0;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt30->branch);
	bit_pos += 1;

	rv_etrace_write_bits(payload->data, bit_pos, params->itrace.privilege_width_p,
			     fmt30->privilege);
	bit_pos += params->itrace.privilege_width_p;

	if (!params->itrace.notime_p) {
		rv_etrace_write_bits(payload->data, bit_pos, params->itrace.time_width_p,
				     fmt30->time);
		bit_pos += params->itrace.time_width_p;
	}

	if (!params->itrace.nocontext_p) {
		rv_etrace_write_bits(payload->data, bit_pos, params->itrace.context_width_p,
				     fmt30->context);
		bit_pos += params->itrace.context_width_p;
	}

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	rv_etrace_write_bits_ll(payload->data, bit_pos, bit_len,
				fmt30->address >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;

	return 0;
}

unsigned int rv_itrace_payload_bits(const struct rv_etrace_params *params,
				    const struct rv_itrace_data *it)
{
	unsigned ret = params->packet.type_width_p;
	int f0s_width_p = params->itrace.f0s_width_p;

	ret += RV_ITRACE_FORMAT_BITS;

	switch (it->format) {
	case 0:
		ret += f0s_width_p;
		switch (it->format0.subformat) {
		case 0:
			ret +=  rv_itrace_format00_bits(params, it);
			break;
		case 1:
			ret +=  rv_itrace_format01_bits(params, it);
			break;
		default:
			break;
		}
		break;
	case 1:
		ret += rv_itrace_format1_bits(params, it);
		break;
	case 2:
		ret += rv_itrace_format2_bits(params);
		break;
	case 3:
		ret += RV_ITRACE_SUBFORMAT_BITS;
		switch (it->format3.subformat) {
		case 0:
			ret += rv_itrace_format30_bits(params);
			break;
		case 1:
			ret += rv_itrace_format31_bits(params, &it->format3.format31);
			break;
		case 2:
			ret += rv_itrace_format32_bits(params);
			break;
		case 3:
			ret += rv_itrace_format33_bits(params);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return ret;
}

int rv_itrace_payload_read(const struct rv_etrace_params *params,
			   const struct rv_etrace_payload *payload,
			   struct rv_itrace_data *it)
{
	int f0s_width_p = params->itrace.f0s_width_p;
	unsigned int bit_pos;
	int rc;

	if (rv_etrace_payload_type_read(params, payload) !=
	    RV_ETRACE_PAYLOAD_TYPE_ITRACE)
		return -1;

	bit_pos = params->packet.type_width_p;
	it->format = rv_etrace_read_bits(payload->data, bit_pos,
					 RV_ITRACE_FORMAT_BITS);
	bit_pos += RV_ITRACE_FORMAT_BITS;

	rc = -1;
	switch (it->format) {
	case 0:
		it->format0.subformat = rv_etrace_read_bits(payload->data, bit_pos,
							    f0s_width_p);
		bit_pos += f0s_width_p;
		switch (it->format0.subformat) {
		case 0:
			rc = rv_itrace_format00_read(params, payload, bit_pos,
						&it->format0.format00);
			break;
		case 1:
			rc = rv_itrace_format01_read(params, payload, bit_pos,
						&it->format0.format01);
			break;
		default:
			break;
		}
		break;
	case 1:
		rc = rv_itrace_format1_read(params, payload, bit_pos, &it->format1);
		break;
	case 2:
		rc = rv_itrace_format2_read(params, payload, bit_pos, &it->format2);
		break;
	case 3:
		it->format3.subformat = rv_etrace_read_bits(payload->data, bit_pos,
							    RV_ITRACE_SUBFORMAT_BITS);
		bit_pos += RV_ITRACE_SUBFORMAT_BITS;

		switch (it->format3.subformat) {
		case 0:
			rc = rv_itrace_format30_read(params, payload, bit_pos,
						     &it->format3.format30);
			break;
		case 1:
			rc = rv_itrace_format31_read(params, payload, bit_pos,
						     &it->format3.format31);
			break;
		case 2:
			rc = rv_itrace_format32_read(params, payload, bit_pos,
						     &it->format3.format32);
			break;
		case 3:
			rc = rv_itrace_format33_read(params, payload, bit_pos,
						     &it->format3.format33);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return rc;
}

int rv_itrace_payload_write(const struct rv_etrace_params *params,
			    struct rv_etrace_payload *payload,
			    const struct rv_itrace_data *it)
{
	unsigned int bit_pos;
	int rc;

	rv_etrace_payload_type_write(params, payload,
				     RV_ETRACE_PAYLOAD_TYPE_ITRACE);
	bit_pos = params->packet.type_width_p;

	rv_etrace_write_bits(payload->data, bit_pos,
			     RV_ITRACE_FORMAT_BITS,
			     it->format);
	bit_pos += RV_ITRACE_FORMAT_BITS;

	payload->size = (rv_itrace_payload_bits(params, it) + 7) >> 3;

	rc = -1;
	switch (it->format) {
	case 0:
		rv_etrace_write_bits(payload->data, bit_pos,
				     RV_ITRACE_SUBFORMAT_BITS,
				     it->format0.subformat);
		bit_pos += RV_ITRACE_SUBFORMAT_BITS;
		switch (it->format0.subformat) {
		case 0:
			rc = rv_itrace_format00_write(params, payload, bit_pos,
						&it->format0.format00);
			break;
		case 1:
			rc = rv_itrace_format01_write(params, payload, bit_pos,
						&it->format0.format01);
			break;
		default:
			break;
		}
		break;
	case 1:
		rc = rv_itrace_format1_write(params, payload, bit_pos, &it->format1);
		break;
	case 2:
		rc = rv_itrace_format2_write(params, payload, bit_pos, &it->format2);
		break;
	case 3:
		rv_etrace_write_bits(payload->data, bit_pos,
				     RV_ITRACE_SUBFORMAT_BITS,
				     it->format3.subformat);
		bit_pos += RV_ITRACE_SUBFORMAT_BITS;

		switch (it->format3.subformat) {
		case 0:
			rc = rv_itrace_format30_write(params, payload, bit_pos,
						      &it->format3.format30);
			break;
		case 1:
			rc = rv_itrace_format31_write(params, payload, bit_pos,
						      &it->format3.format31);
			break;
		case 2:
			rc = rv_itrace_format32_write(params, payload, bit_pos,
						      &it->format3.format32);
			break;
		case 3:
			rc = rv_itrace_format33_write(params, payload, bit_pos,
						      &it->format3.format33);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return rc;
}
