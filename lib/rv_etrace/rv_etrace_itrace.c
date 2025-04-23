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
	if (branches <= 1)
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
	fmt1->branch_map = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
	bit_pos += bit_len;

	if (bit_len == 31)
		return 0;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	fmt1->address = rv_etrace_read_bits_ll(payload->data, bit_pos, bit_len);
	fmt1->address = fmt1->address << params->itrace.iaddress_lsb_p;
	bit_pos += bit_len;

	fmt1->notify = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;
	fmt1->updiscon = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;
	fmt1->irreport = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	fmt1->irdepth = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
	bit_pos += bit_len;

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
				fmt1->address >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt1->notify);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt1->updiscon);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt1->irreport);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt1->irdepth);
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
	unsigned int bit_len = 0;

	bit_len = params->itrace.iaddress_width_p - params->itrace.iaddress_lsb_p;
	fmt2->address = rv_etrace_read_bits_ll(payload->data, bit_pos, bit_len);
	fmt2->address = fmt2->address << params->itrace.iaddress_lsb_p;
	bit_pos += bit_len;

	fmt2->notify = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;
	fmt2->updiscon = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;
	fmt2->irreport = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				       params->itrace.call_counter_size_p);
	fmt2->irdepth = rv_etrace_read_bits(payload->data, bit_pos, bit_len);
	bit_pos += bit_len;

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
				fmt2->address >> params->itrace.iaddress_lsb_p);
	bit_pos += bit_len;

	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt2->notify);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt2->updiscon);
	bit_pos += 1;
	rv_etrace_write_bits(payload->data, bit_pos, 1, fmt2->irreport);
	bit_pos += 1;

	bit_len = calculate_irdepth(params->itrace.return_stack_size_p,
				    params->itrace.call_counter_size_p);
	rv_etrace_write_bits(payload->data, bit_pos, bit_len, fmt2->irdepth);
	bit_pos += bit_len;

	return 0;
}

/** Function to get number of bits required by itrace format=3 sub-format=3 */
static unsigned int rv_itrace_format33_bits(const struct rv_etrace_params *params)
{
	unsigned int ret = 1;

	ret += 32; /** N bits: Encoder mode (assuming 32bits ) */
	ret += 2;
	ret += 32;  /** N bits: Instruction options (assuming 32 bits ) */
	ret += 1;
	ret += 1;
	ret += 32;  /** M bits: Data options (assuming 32 bits ) */

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

	fmt33->encoder_mode = rv_etrace_read_bits(payload->data, bit_pos, 32);
	bit_pos += 32;

	fmt33->qual_status = rv_etrace_read_bits(payload->data, bit_pos, 2);
	bit_pos += 2;

	fmt33->ioptions = rv_etrace_read_bits(payload->data, bit_pos, 32);
	bit_pos += 32;

	fmt33->denable = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	fmt33->dloss = rv_etrace_read_bits(payload->data, bit_pos, 1);
	bit_pos += 1;

	fmt33->doptions = rv_etrace_read_bits(payload->data, bit_pos, 32);
	bit_pos += 32;

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
	if (!fmt31->interrupt)
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
	unsigned long bit_len = 0;

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

	ret += RV_ITRACE_FORMAT_BITS;

	switch (it->format) {
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
	unsigned int size, bit_pos;
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

	size = (rv_itrace_payload_bits(params, it) + 7) >> 3;
	if (size > payload->size)
		return -1;

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
