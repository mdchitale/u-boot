/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Macros and helper routines to access instruction trace payload
 * based on the ratified v2.0.3 RISC-V etrace specification [1].
 *
 * [1] https://github.com/riscv-non-isa/riscv-trace-spec
 */

#ifndef __RV_ETRACE_ITRACE_H__
#define __RV_ETRACE_ITRACE_H__

#include <rv_etrace_params.h>
#include <rv_etrace_payload.h>

#define RV_ITRACE_FORMAT_BITS		2
#define RV_ITRACE_SUBFORMAT_BITS	2

struct rv_itrace_iaddress {
	unsigned long long addr; /** Address (calculated from iaddress_width_p and iaddress_lsb_p) */
	unsigned int notify;   /**  Notify flag (1  bit)  */
	unsigned int updiscon; /**  Up Disconnect flag (1  bit) */
	unsigned int irreport; /**  Instruction Report flag (1  bit) */
	unsigned int irdepth;  /**  IR Depth (calculated dynamically)  */
};

/** Itrace format=0 and subfromat 1  */
struct rv_itrace_format01 {
	unsigned int index;	/** cache_size_p */
	unsigned int branches;   /** 5 bits -- number of valid bits in branch_map */
	unsigned int branch_map; /** Branch map (31 bits) */
	unsigned int irreport;		/** Instruction Report flag (1  bit) */
	unsigned int irdepth;		/** IR Depth (calculated dynamically)  */
};

/** Itrace format=0 and subfromat 0  address, branch map*/
struct rv_itrace_format00 {
	unsigned int branch_count;	/** 32 bit */
	unsigned int branch_fmt;	/**2bit -- 00 for no address */
	struct rv_itrace_iaddress iaddress;
};

struct rv_itrace_format0 {
	unsigned int subformat;
	union {
		struct rv_itrace_format00 format00;
		struct rv_itrace_format01 format01;
	};
};


/** Itrace format=1 data */
struct rv_itrace_format1 {
	unsigned int branches;		/** 5 bits -- number of valid bits in branch_map */
	unsigned int branch_map;	/** Branch map (31 bits) */
	struct rv_itrace_iaddress iaddress;
};

/** Itrace format=2 data */
struct rv_itrace_format2 {
	struct rv_itrace_iaddress iaddress;
};

/** Itrace format=3 sub-format=2 data */
struct rv_itrace_format32 {
	unsigned int privilege;	/** Privilege level (as a bit field or enum value) */
	unsigned int time;	/** Time field (set to 0 if notime_p is true) */
	unsigned int context;	/** Context field (set to 0 if nocontext_p is true) */
};

/** Itrace format=3 sub-format=1 data */
struct rv_itrace_format31 {
	unsigned int branch;		/** 1 bit */
	unsigned int privilege;		/** Privilege level (as a bit field or enum value) */
	unsigned int time;		/** Time field (set to 0 if notime_p is true) */
	unsigned int context;		/** Context field (set to 0 if nocontext_p is true) */
	unsigned int ecause;		/** ecause_width_p   -- 6-bits */
	unsigned int interrupt;		/** 1 bit  */
	unsigned int thaddr;		/** 1 bit */
	unsigned long long address;	/** Address (calculated from iaddress_width_p and iaddress_lsb_p) */
	unsigned long long tval;	/** iaddress_width_p -- 50bit as per config */
};

/** Itrace format=3 sub-format=0 data */
struct rv_itrace_format30 {
	unsigned int branch;		/** 1 bit */
	unsigned int privilege;	/** Privilege level (as a bit field or enum value) */
	unsigned int time;	/** Time field (set to 0 if notime_p is true) */
	unsigned int context;	/** Context field (set to 0 if nocontext_p is true) */
	unsigned long long address;/** Address (calculated from iaddress_width_p and iaddress_lsb_p) */
};

/** Itrace format=3 sub-format=3 data */
struct rv_itrace_format33 {
	unsigned int ienable;		/** 1 bit: Instruction enable flag */
	unsigned int encoder_mode;	/** N bits: Encoder mode (assuming 32bits ) */
	unsigned int qual_status;	/** 2 bits: Qualifier status */
	unsigned int ioptions;		/** N bits: Instruction options (assuming 32 bits ) */
	unsigned int denable;		/** 1 bit: Data enable flag */
	unsigned int dloss;		/** 1 bit: Data loss flag */
	unsigned int doptions;		/** M bits: Data options (assuming 32 bits ) */
} ;

/** Itrace format=3 data */
struct rv_itrace_format3 {
	unsigned int subformat;		/** Subformat field */
	union {
		struct rv_itrace_format30 format30;
		struct rv_itrace_format31 format31;
		struct rv_itrace_format32 format32;
		struct rv_itrace_format33 format33;
	};
};

/** Itrace data */
struct rv_itrace_data {
	unsigned int format;		/* Format field */
	union {
		struct rv_itrace_format0 format0;
		struct rv_itrace_format1 format1;
		struct rv_itrace_format2 format2;
		struct rv_itrace_format3 format3;
	};
};

/** Function to get number of bits required by itrace payload data */
unsigned int rv_itrace_payload_bits(const struct rv_etrace_params *params,
				    const struct rv_itrace_data *it);

/** Function to decode itrace payload data */
int rv_itrace_payload_read(const struct rv_etrace_params *params,
			   const struct rv_etrace_payload *payload,
			   struct rv_itrace_data *it);

/** Function to encode itrace payload data */
int rv_itrace_payload_write(const struct rv_etrace_params *params,
			    struct rv_etrace_payload *payload,
			    const struct rv_itrace_data *it);

#endif
