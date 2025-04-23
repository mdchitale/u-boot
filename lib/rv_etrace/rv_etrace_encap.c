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

#include <rv_etrace_encap.h>

int rv_etrace_packet_srcid_read(const struct rv_etrace_params *params,
				const struct rv_etrace_packet *pkt,
				unsigned int *srcid)
{
	int i, index = rv_etrace_packet_srcid_index(params, pkt);

	if (index < 0)
		return index;

	*srcid = 0;
	for (i = 0; i < params->packet.srcid_bytes_p; i++)
		*srcid |= (unsigned int)pkt->data[index + i] << (i * 8);
	return i;
}

int rv_etrace_packet_srcid_write(const struct rv_etrace_params *params,
				 struct rv_etrace_packet *pkt,
				 unsigned int srcid)
{
	int i, index = rv_etrace_packet_srcid_index(params, pkt);

	if (index < 0)
		return index;

	for (i = 0; i < params->packet.srcid_bytes_p; i++)
		pkt->data[index + i] = (srcid >> (i * 8)) & 0xff;
	return i;
}

int rv_etrace_packet_tstamp_read(const struct rv_etrace_params *params,
				 const struct rv_etrace_packet *pkt,
				 unsigned long long *tstamp)
{
	int i, index = rv_etrace_packet_tstamp_index(params, pkt);

	if (index < 0)
		return index;

	*tstamp = 0;
	for (i = 0; i < params->packet.tstamp_bytes_p; i++)
		*tstamp |= (unsigned long long)pkt->data[index + i] << (i * 8);
	return i;
}

int rv_etrace_packet_tstamp_write(const struct rv_etrace_params *params,
				  struct rv_etrace_packet *pkt,
				  unsigned long long tstamp)
{
	int i, index = rv_etrace_packet_tstamp_index(params, pkt);

	if (index < 0)
		return index;

	for (i = 0; i < params->packet.tstamp_bytes_p; i++)
		pkt->data[index + i] = (tstamp >> (i * 8)) & 0xff;
	return i;
}

int rv_etrace_packet_payload_read(const struct rv_etrace_params *params,
				  const struct rv_etrace_packet *pkt,
				  struct rv_etrace_payload *payload)
{
	int i, index = rv_etrace_packet_payload_index(params, pkt);

	if (index < 0)
		return index;

	payload->size = rv_etrace_packet_header_length_read(pkt);
	for (i = 0; i < payload->size; i++)
		payload->data[i] = pkt->data[index + i];
	return i;
}

int rv_etrace_packet_payload_write(const struct rv_etrace_params *params,
				   struct rv_etrace_packet *pkt,
				   const struct rv_etrace_payload *payload)
{
	int i, index = rv_etrace_packet_payload_index(params, pkt);

	if (index < 0)
		return index;

	rv_etrace_packet_header_length_write(pkt, payload->size);
	for (i = 0; i < rv_etrace_packet_header_length_read(pkt); i++)
		pkt->data[index + i] = payload->data[i];
	return i;
}

unsigned int rv_etrace_packet_size(const struct rv_etrace_params *params,
				   const struct rv_etrace_packet *pkt)
{
	return RV_ETRACE_HEADER_BYTES +
		params->packet.srcid_bytes_p +
		(rv_etrace_packet_header_extend_read(pkt) ? params->packet.tstamp_bytes_p : 0) +
		rv_etrace_packet_header_length_read(pkt);
}

int rv_etrace_packet_write(const struct rv_etrace_params *params,
			   struct rv_etrace_packet *pkt,
			   unsigned int flow, unsigned int srcid,
			   unsigned int extend, unsigned long long tstamp,
			   const struct rv_etrace_payload *payload)
{
	int rc, ret = 0;

	rc = rv_etrace_packet_header_write(pkt, flow,
					   params->packet.tstamp_bytes_p && extend ? 1 : 0,
					   0);
	if (rc < 0)
		return rc;
	ret += rc;

	rc = rv_etrace_packet_srcid_write(params, pkt, srcid);
	if (rc < 0)
		return rc;
	ret += rc;

	rv_etrace_packet_tstamp_write(params, pkt, tstamp);
	if (rc < 0)
		return rc;
	ret += rc;

	rv_etrace_packet_payload_write(params, pkt, payload);
	if (rc < 0)
		return rc;
	ret += rc;

	return ret;
}
