/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Various implementation specific RISC-V etrace parameters based
 * on the ratified v2.0.3 RISC-V etrace specification [1] and the
 * ratified v1.0.0 RISC-V etrace encapsulation specification [2].
 *
 * [1] https://github.com/riscv-non-isa/riscv-trace-spec
 * [2] https://github.com/riscv-non-isa/e-trace-encap
 */

#ifndef __RV_ETRACE_PARAMS_H__
#define __RV_ETRACE_PARAMS_H__

/* Maximum possible sizes of source ID at packet level. */
#define RV_ETRACE_SRCID_MAX_BYTES		2

/* Maximum possible size of timestamp at packet level. */
#define RV_ETRACE_TSTAMP_MAX_BYTES		8

/* Maximum possible width of type at packet level. */
#define RV_ETRACE_TYPE_MAX_WIDTH		1

/* Maximum possible size of payload at packet level */
#define RV_ETRACE_PAYLOAD_MAX_BYTES		31

/* Packet level parameters */
struct rv_etrace_packet_params {
	/* Number of bytes for source ID (maximum 2) */
	unsigned int srcid_bytes_p;
	/* Number of timestamp bytes (maximum 8) */
	unsigned int tstamp_bytes_p;
	/* Number of type bits */
	unsigned int type_width_p;
};

/* Payload level intruction trace paratmeters */
struct rv_etrace_itrace_params {
	/*
	 * The architecture specification version with which the encoder
	 * is compliant (0 for initial version).
	 */
	unsigned int arch_p;
	/* Number of times iretire, itype etc. are replicated */
	unsigned int blocks_p;
	/*
	 * Number of entries in the branch predictor is 2bpred_size_p.
	 * Minimum number of entries is 2, so a value of 0 indicates
	 * that there is no branch predictor implemented.
	 */
	unsigned int bpred_size_p;
	/*
	 * Number of entries in the jump target cache is 2cache_size_p.
	 * Minimum number of entries is 2, so a value of 0 indicates
	 * that there is no jump target cache implemented.
	 */
	unsigned int cache_size_p;
	/*
	 * Number of bits in the nested call counter is 2call_counter_size_p.
	 * Minimum number of entries is 2, so a value of 0 indicates that
	 * there is no implicit return call counter implemented.
	 */
	unsigned int call_counter_size_p;
	/* Width of the ctype bus */
	unsigned int ctype_width_p;
	/* Width of context bus */
	unsigned int context_width_p;
	/* Width of time bus */
	unsigned int time_width_p;
	/* Width of exception cause bus */
	unsigned int ecause_width_p;
	/* Number of bits of exception cause to match using multiple choice */
	unsigned int ecause_choice_p;
	/*
	 * Width of the subformat field in format 0 te_inst packets
	 * (see Section 7.8.1).
	 */
	unsigned int f0s_width_p;
	/* 0 or 1 Filtering on context supported when 1 */
	unsigned int filter_context_p;
	/* 0 or 1 Filtering on time supported when 1 */
	unsigned int filter_time_p;
	/*
	 * Filtering on exception cause or interrupt supported when non_zero.
	 * Number of nested exceptions supported is 2filter_excint_p
	 */
	unsigned int filter_excint_p;
	/* 0 or 1 Filtering on privilege supported when 1 */
	unsigned int filter_privilege_p;
	/*
	 * 0 or 1 Filtering on trap value supported when 1 (provided
	 * filter_excint_p is non-zero)
	 */
	unsigned int filter_tval_p;
	/*
	 * LSB of instruction address bus to trace. 1 is compressed
	 * instructions are supported, 2 otherwise
	 */
	unsigned int iaddress_lsb_p;
	/* Width of instruction address bus. This is the same as DXLEN */
	unsigned int iaddress_width_p;
	/* Width of the iretire bus */
	unsigned int iretire_width_p;
	/* Width of the ilastsize bus */
	unsigned int ilastsize_width_p;
	/* Width of the itype bus */
	unsigned int itype_width_p;
	/* 0 or 1 Exclude context from te_inst packets if 1 */
	unsigned int nocontext_p;
	/* 0 or 1 Exclude time from te_inst packets if 1 */
	unsigned int notime_p;
	/* Width of privilege bus */
	unsigned int privilege_width_p;
	/* Maximum number of instructions that can be retired per block */
	unsigned int retires_p;
	/*
	 * Number of entries in the return address stack is 2return_stack_size_p.
	 * Minimum number of entries is 2, so a value of 0 indicates that there
	 * is no implicit return stack implemented.
	 */
	unsigned int return_stack_size_p;
	/* 0 or 1 sijump is used to identify sequentially inferable jumps */
	unsigned int sijump_p;
	/* Width of implementation-defined input bus */
	unsigned int impdef_width_p;
};

/* Payload level data trace paratmeters */
struct rv_etrace_dtrace_params {
	/* Width of the daddress bus */
	unsigned int daddress_width_p;
	/* Width of the dblock bus */
	unsigned int dblock_width_p;
	/* Width of the data bus */
	unsigned int data_width_p;
	/* Width of the dsize bus */
	unsigned int dsize_width_p;
	/* Width of the dtype bus */
	unsigned int dtype_width_p;
	/* Width of the iaddr_lsbs bus */
	unsigned int iaddr_lsbs_width_p;
	/* Width of the lrid bus */
	unsigned int lrid_width_p;
	/* Width of the lresp bus */
	unsigned int lresp_width_p;
	/* Width of the ldata bus */
	unsigned int ldata_width_p;
	/* Width of the sdata bus */
	unsigned int sdata_width_p;
};

struct rv_etrace_params {
	struct rv_etrace_packet_params packet;
	struct rv_etrace_itrace_params itrace;
	struct rv_etrace_dtrace_params dtrace;
};

static inline int rv_etrace_packet_params_valid(const struct rv_etrace_packet_params *params)
{
	if (params->srcid_bytes_p > RV_ETRACE_SRCID_MAX_BYTES ||
	    params->tstamp_bytes_p > RV_ETRACE_TSTAMP_MAX_BYTES ||
	    params->type_width_p > RV_ETRACE_TYPE_MAX_WIDTH)
		return 0;

	return 1;
}

static inline int rv_etrace_params_valid(const struct rv_etrace_params *params)
{
	if (!rv_etrace_packet_params_valid(&params->packet))
		return 0;

	/* TODO: check itrace and dtrace parameters */
	return 1;
}

#endif
