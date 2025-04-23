/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Macros, data structures, and helper routines based on the
 * ratified v1.0.0 RISC-V etrace encapsulation specification [1].
 *
 * [1] https://github.com/riscv-non-isa/e-trace-encap
 */

#ifndef __RV_ETRACE_ENCAP_H__
#define __RV_ETRACE_ENCAP_H__

#include <rv_etrace_bits.h>
#include <rv_etrace_params.h>
#include <rv_etrace_payload.h>

#define RV_ETRACE_HEADER_BYTES			1U

#define RV_ETRACE_MAX_DATA_BYTES		\
	(RV_ETRACE_SRCID_MAX_BYTES +		\
	 RV_ETRACE_TSTAMP_MAX_BYTES +		\
	 RV_ETRACE_PAYLOAD_MAX_BYTES)

#define RV_ETRACE_MAX_PACKET_BYTES		\
	(RV_ETRACE_HEADER_BYTES + RV_ETRACE_MAX_DATA_BYTES)

#define RV_ETRACE_HEADER_LENGTH_BITS		5
#define RV_ETRACE_HEADER_LENGTH_MASK		\
	((1U << RV_ETRACE_HEADER_LENGTH_BITS) - 1)
#define RV_ETRACE_HEADER_LENGTH_SHIFT		0
#define RV_ETRACE_HEADER_LENGTH			\
	(RV_ETRACE_HEADER_LENGTH_MASK << RV_ETRACE_HEADER_LENGTH_SHIFT)

#define RV_ETRACE_HEADER_FLOW_BITS		2
#define RV_ETRACE_HEADER_FLOW_MASK		\
	((1U << RV_ETRACE_HEADER_FLOW_BITS) - 1)
#define RV_ETRACE_HEADER_FLOW_SHIFT		RV_ETRACE_HEADER_LENGTH_BITS
#define RV_ETRACE_HEADER_FLOW			\
	(RV_ETRACE_HEADER_FLOW_MASK << RV_ETRACE_HEADER_FLOW_SHIFT)

#define RV_ETRACE_HEADER_EXTEND_BITS		1
#define RV_ETRACE_HEADER_EXTEND_MASK		\
	((1U << RV_ETRACE_HEADER_EXTEND_BITS) - 1)
#define RV_ETRACE_HEADER_EXTEND_SHIFT		\
	(RV_ETRACE_HEADER_LENGTH_BITS + RV_ETRACE_HEADER_FLOW_BITS)
#define RV_ETRACE_HEADER_EXTEND			\
	(RV_ETRACE_HEADER_EXTEND_MASK << RV_ETRACE_HEADER_EXTEND_SHIFT)

struct rv_etrace_packet {
	unsigned char header;
	unsigned char data[RV_ETRACE_MAX_DATA_BYTES];
};

static inline unsigned int rv_etrace_packet_header_length_read(
					const struct rv_etrace_packet *pkt)
{
	return rv_etrace_read_bits(&pkt->header, RV_ETRACE_HEADER_LENGTH_SHIFT,
				   RV_ETRACE_HEADER_LENGTH_BITS);
}

static inline void rv_etrace_packet_header_length_write(struct rv_etrace_packet *pkt,
							unsigned int length)
{
	if (length > RV_ETRACE_PAYLOAD_MAX_BYTES)
		length = RV_ETRACE_PAYLOAD_MAX_BYTES;

	rv_etrace_write_bits(&pkt->header, RV_ETRACE_HEADER_LENGTH_SHIFT,
			     RV_ETRACE_HEADER_LENGTH_BITS, length);
}

static inline unsigned int rv_etrace_packet_header_flow_read(
					const struct rv_etrace_packet *pkt)
{
	return rv_etrace_read_bits(&pkt->header, RV_ETRACE_HEADER_FLOW_SHIFT,
				   RV_ETRACE_HEADER_FLOW_BITS);
}

static inline void rv_etrace_packet_header_flow_write(struct rv_etrace_packet *pkt,
						      unsigned int flow)
{
	rv_etrace_write_bits(&pkt->header, RV_ETRACE_HEADER_FLOW_SHIFT,
			     RV_ETRACE_HEADER_FLOW_BITS, flow);
}

static inline unsigned int rv_etrace_packet_header_extend_read(
						const struct rv_etrace_packet *pkt)
{
	return pkt->header & RV_ETRACE_HEADER_EXTEND ? 1 : 0;
}

static inline void rv_etrace_packet_header_extend_write(struct rv_etrace_packet *pkt,
							unsigned int extend)
{
	if (extend)
		pkt->header |= RV_ETRACE_HEADER_EXTEND;
	else
		pkt->header &= ~RV_ETRACE_HEADER_EXTEND;
}

static inline int rv_etrace_packet_header_write(struct rv_etrace_packet *pkt,
						unsigned int flow,
						unsigned int extend,
						unsigned int length)
{
	pkt->header = 0;
	rv_etrace_packet_header_length_write(pkt, length);
	rv_etrace_packet_header_flow_write(pkt, flow);
	rv_etrace_packet_header_extend_write(pkt, extend);
	return 1;
}

static inline int rv_etrace_packet_srcid_index(const struct rv_etrace_params *params,
					       const struct rv_etrace_packet *pkt)
{
	if (!rv_etrace_packet_params_valid(&params->packet) ||
	    !params->packet.srcid_bytes_p)
		return -1;

	return 0;
}

int rv_etrace_packet_srcid_read(const struct rv_etrace_params *params,
				const struct rv_etrace_packet *pkt,
				unsigned int *srcid);

int rv_etrace_packet_srcid_write(const struct rv_etrace_params *params,
				 struct rv_etrace_packet *pkt,
				 unsigned int srcid);

static inline int rv_etrace_packet_tstamp_index(const struct rv_etrace_params *params,
					 	const struct rv_etrace_packet *pkt)
{
	if (!rv_etrace_packet_params_valid(&params->packet) ||
	    !rv_etrace_packet_header_extend_read(pkt) ||
	    !params->packet.tstamp_bytes_p)
		return -1;

	return params->packet.srcid_bytes_p;
}

int rv_etrace_packet_tstamp_read(const struct rv_etrace_params *params,
				 const struct rv_etrace_packet *pkt,
				 unsigned long long *tstamp);

int rv_etrace_packet_tstamp_write(const struct rv_etrace_params *params,
				  struct rv_etrace_packet *pkt,
				  unsigned long long tstamp);

static inline int rv_etrace_packet_payload_index(const struct rv_etrace_params *params,
						 const struct rv_etrace_packet *pkt)
{
	if (!rv_etrace_packet_params_valid(&params->packet))
		return -1;

	return !rv_etrace_packet_header_extend_read(pkt) ? params->packet.srcid_bytes_p :
		params->packet.srcid_bytes_p + params->packet.tstamp_bytes_p;
}

int rv_etrace_packet_payload_read(const struct rv_etrace_params *params,
				  const struct rv_etrace_packet *pkt,
				  struct rv_etrace_payload *payload);

int rv_etrace_packet_payload_write(const struct rv_etrace_params *params,
				   struct rv_etrace_packet *pkt,
				   const struct rv_etrace_payload *payload);

unsigned int rv_etrace_packet_size(const struct rv_etrace_params *params,
				   const struct rv_etrace_packet *pkt);

int rv_etrace_packet_write(const struct rv_etrace_params *params,
			   struct rv_etrace_packet *pkt,
			   unsigned int flow, unsigned int srcid,
			   unsigned int extend, unsigned long long tstamp,
			   const struct rv_etrace_payload *payload);

#endif
