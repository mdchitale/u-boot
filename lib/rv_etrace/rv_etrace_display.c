/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR BSD-2-Clause
 *
 * Copyright (c) 2025 Ventana Micro Systems Inc.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vsprintf.h>
#include <rv_etrace_encap.h>
#include <rv_etrace_itrace.h>
#include <rv_etrace_display.h>

static struct rv_etrace_params params_g = { 0 };

void display_params(void)
{
	printf("Packet Parameters:\n");
	printf("\tsrcid_bytes_p: %d\n", params_g.packet.srcid_bytes_p);
	printf("\ttstamp_bytes_p: %d\n", params_g.packet.tstamp_bytes_p);
	printf("\ttype_width_p: %d\n", params_g.packet.type_width_p);

	printf("\nItrace Parameters:\n");
	printf("\tarch_p: %d\n", params_g.itrace.arch_p);
	printf("\tblocks_p: %d\n", params_g.itrace.blocks_p);
	printf("\tbpred_size_p: %d\n", params_g.itrace.bpred_size_p);
	printf("\tcache_size_p: %d\n", params_g.itrace.cache_size_p);
	printf("\tcall_counter_size_p: %d\n", params_g.itrace.call_counter_size_p);
	printf("\tctype_width_p: %d\n", params_g.itrace.ctype_width_p);
	printf("\tcontext_width_p: %d\n", params_g.itrace.context_width_p);
	printf("\ttime_width_p: %d\n", params_g.itrace.time_width_p);
	printf("\tecause_width_p: %d\n", params_g.itrace.ecause_width_p);
	printf("\tf0s_width_p: %d\n", params_g.itrace.f0s_width_p);
	printf("\tfilter_context_p: %d\n", params_g.itrace.filter_context_p);
	printf("\tfilter_time_p: %d\n", params_g.itrace.filter_time_p);
	printf("\tfilter_excint_p: %d\n", params_g.itrace.filter_excint_p);
	printf("\tfilter_privilege_p: %d\n", params_g.itrace.filter_privilege_p);
	printf("\tfilter_tval_p: %d\n", params_g.itrace.filter_tval_p);
	printf("\tiaddress_lsb_p: %d\n", params_g.itrace.iaddress_lsb_p);
	printf("\tiaddress_width_p: %d\n", params_g.itrace.iaddress_width_p);
	printf("\tiretire_width_p: %d\n", params_g.itrace.iretire_width_p);
	printf("\tilastsize_width_p: %d\n", params_g.itrace.ilastsize_width_p);
	printf("\titype_width_p: %d\n", params_g.itrace.itype_width_p);
	printf("\tnocontext_p: %d\n", params_g.itrace.nocontext_p);
	printf("\tnotime_p: %d\n", params_g.itrace.notime_p);
	printf("\tprivilege_width_p: %d\n", params_g.itrace.privilege_width_p);
	printf("\tretires_p: %d\n", params_g.itrace.retires_p);
	printf("\treturn_stack_size_p: %d\n", params_g.itrace.return_stack_size_p);
	printf("\tsijump_p: %d\n", params_g.itrace.sijump_p);
	printf("\timpdef_width_p: %d\n", params_g.itrace.impdef_width_p);

	printf("\nDtrace Parameters:\n");
	printf("\tdaddress_width_p: %d\n", params_g.dtrace.daddress_width_p);
	printf("\tdblock_width_p: %d\n", params_g.dtrace.dblock_width_p);
	printf("\tdata_width_p: %d\n", params_g.dtrace.data_width_p);
	printf("\tdsize_width_p: %d\n", params_g.dtrace.dsize_width_p);
	printf("\tdtype_width_p: %d\n", params_g.dtrace.dtype_width_p);
	printf("\tiaddr_lsbs_width_p: %d\n", params_g.dtrace.iaddr_lsbs_width_p);
	printf("\tlrid_width_p: %d\n", params_g.dtrace.lrid_width_p);
	printf("\tlresp_width_p: %d\n", params_g.dtrace.lresp_width_p);
	printf("\tldata_width_p: %d\n", params_g.dtrace.ldata_width_p);
	printf("\tsdata_width_p: %d\n", params_g.dtrace.sdata_width_p);

	printf("\n");
}

int rv_etrace_parse_params(const char *itrace_params[MAX_LINE_LENGTH],
			   bool show_params)
{
	char param[MAX_PARAM_LENGTH];
	const char *line;
	int value, i = 0;

	while (strlen(itrace_params[i])) {
		line = itrace_params[i];
		/* Parse the parameter and value */
		if (sscanf(line, "%127[^,],%d", param, &value) != 2) {
			printf("%s: error parsing line \"%s\"\n", __func__, line);
			return -EINVAL;
		}

		/* Check and assign the value to the appropriate struct field */
		if (strcmp(param, ".packet.srcid_bytes_p") == 0) {
			params_g.packet.srcid_bytes_p = value;
		} else if (strcmp(param, ".packet.tstamp_bytes_p") == 0) {
			params_g.packet.tstamp_bytes_p = value;
		} else if (strcmp(param, ".packet.type_width_p") == 0) {
			params_g.packet.type_width_p = value;
		} else if (strcmp(param, ".itrace.arch_p") == 0) {
			params_g.itrace.arch_p = value;
		} else if (strcmp(param, ".itrace.blocks_p") == 0) {
			params_g.itrace.blocks_p = value;
		} else if (strcmp(param, ".itrace.bpred_size_p") == 0) {
			params_g.itrace.bpred_size_p = value;
		} else if (strcmp(param, ".itrace.cache_size_p") == 0) {
			params_g.itrace.cache_size_p = value;
		} else if (strcmp(param, ".itrace.call_counter_size_p") == 0) {
			params_g.itrace.call_counter_size_p = value;
		} else if (strcmp(param, ".itrace.ctype_width_p") == 0) {
			params_g.itrace.ctype_width_p = value;
		} else if (strcmp(param, ".itrace.context_width_p") == 0) {
			params_g.itrace.context_width_p = value;
		} else if (strcmp(param, ".itrace.time_width_p") == 0) {
			params_g.itrace.time_width_p = value;
		} else if (strcmp(param, ".itrace.ecause_width_p") == 0) {
			params_g.itrace.ecause_width_p = value;
		} else if (strcmp(param, ".itrace.f0s_width_p") == 0) {
			params_g.itrace.f0s_width_p = value;
		} else if (strcmp(param, ".itrace.filter_context_p") == 0) {
			params_g.itrace.filter_context_p = value;
		} else if (strcmp(param, ".itrace.filter_time_p") == 0) {
			params_g.itrace.filter_time_p = value;
		} else if (strcmp(param, ".itrace.filter_excint_p") == 0) {
			params_g.itrace.filter_excint_p = value;
		} else if (strcmp(param, ".itrace.filter_privilege_p") == 0) {
			params_g.itrace.filter_privilege_p = value;
		} else if (strcmp(param, ".itrace.filter_tval_p") == 0) {
			params_g.itrace.filter_tval_p = value;
		} else if (strcmp(param, ".itrace.iaddress_lsb_p") == 0) {
			params_g.itrace.iaddress_lsb_p = value;
		} else if (strcmp(param, ".itrace.iaddress_width_p") == 0) {
			params_g.itrace.iaddress_width_p = value;
		} else if (strcmp(param, ".itrace.iretire_width_p") == 0) {
			params_g.itrace.iretire_width_p = value;
		} else if (strcmp(param, ".itrace.ilastsize_width_p") == 0) {
			params_g.itrace.ilastsize_width_p = value;
		} else if (strcmp(param, ".itrace.itype_width_p") == 0) {
			params_g.itrace.itype_width_p = value;
		} else if (strcmp(param, ".itrace.nocontext_p") == 0) {
			params_g.itrace.nocontext_p = value;
		} else if (strcmp(param, ".itrace.notime_p") == 0) {
			params_g.itrace.notime_p = value;
		} else if (strcmp(param, ".itrace.privilege_width_p") == 0) {
			params_g.itrace.privilege_width_p = value;
		} else if (strcmp(param, ".itrace.retires_p") == 0) {
			params_g.itrace.retires_p = value;
		} else if (strcmp(param, ".itrace.return_stack_size_p") == 0) {
			params_g.itrace.return_stack_size_p = value;
		} else if (strcmp(param, ".itrace.sijump_p") == 0) {
			params_g.itrace.sijump_p = value;
		} else if (strcmp(param, ".itrace.impdef_width_p") == 0) {
			params_g.itrace.impdef_width_p = value;
		} else if (strcmp(param, ".dtrace.daddress_width_p") == 0) {
			params_g.dtrace.daddress_width_p = value;
		} else if (strcmp(param, ".dtrace.dblock_width_p") == 0) {
			params_g.dtrace.dblock_width_p = value;
		} else if (strcmp(param, ".dtrace.data_width_p") == 0) {
			params_g.dtrace.data_width_p = value;
		} else if (strcmp(param, ".dtrace.dsize_width_p") == 0) {
			params_g.dtrace.dsize_width_p = value;
		} else if (strcmp(param, ".dtrace.dtype_width_p") == 0) {
			params_g.dtrace.dtype_width_p = value;
		} else if (strcmp(param, ".dtrace.iaddr_lsbs_width_p") == 0) {
			params_g.dtrace.iaddr_lsbs_width_p = value;
		} else if (strcmp(param, ".dtrace.lrid_width_p") == 0) {
			params_g.dtrace.lrid_width_p = value;
		} else if (strcmp(param, ".dtrace.lresp_width_p") == 0) {
			params_g.dtrace.lresp_width_p = value;
		} else if (strcmp(param, ".dtrace.ldata_width_p") == 0) {
			params_g.dtrace.ldata_width_p = value;
		} else if (strcmp(param, ".dtrace.sdata_width_p") == 0) {
			params_g.dtrace.sdata_width_p = value;
		}
		i++;
	}

	if (show_params)
		display_params();

	return 0;
}

static void display_itrace_format1(unsigned int in_pkt_num,
				   const struct rv_itrace_data *it)
{
	const struct rv_itrace_format1 *data = &it->format1;

	printf(" format1");
	printf(" branches=%d", data->branches);
	printf(" branche_map=0x%x", data->branch_map);
	if (data->branches == 31)
		return;
	printf(" address=0x%llx", data->address);
	printf(" notify=%u", data->notify);
	printf(" updiscon=%u", data->updiscon);
	printf(" irreport=%u", data->irreport);
	printf(" irdepth=0x%x", data->irdepth);
}

static void display_itrace_format2(unsigned int in_pkt_num,
				   const struct rv_itrace_data *it)
{
	const struct rv_itrace_format2 *data = &it->format2;

	printf(" format2");
	printf(" address=0x%llx", data->address);
	printf(" notify=%u", data->notify);
	printf(" updiscon=%u", data->updiscon);
	printf(" irreport=%u", data->irreport);
	printf(" irdepth=0x%x", data->irdepth);
}

static void display_itrace_format33(unsigned int in_pkt_num,
				    const struct rv_itrace_data *it)
{
	const struct rv_itrace_format33 *data = &it->format3.format33;

	printf(" format3 subformat3");
	printf(" ienable=%u", data->ienable);
	printf(" encoder_mode=0x%x", data->encoder_mode);
	printf(" qual_status=0x%x", data->qual_status);
	printf(" ioptions=0x%x", data->ioptions);
	printf(" denable=%u", data->denable);
	printf(" dloss=%u", data->dloss);
	printf(" doptions=0x%x", data->doptions);
}

static void display_itrace_format32(unsigned int in_pkt_num,
				    const struct rv_itrace_data *it)
{
	const struct rv_itrace_format32 *data = &it->format3.format32;

	printf(" format3 subformat2");
	printf(" privilege=0x%x", data->privilege);
	if (!params_g.itrace.notime_p)
		printf(" time=0x%x", data->time);
	if (!params_g.itrace.nocontext_p)
		printf(" context=0x%x", data->context);
}

static void display_itrace_format31(unsigned int in_pkt_num,
				    const struct rv_itrace_data *it)
{
	const struct rv_itrace_format31 *data = &it->format3.format31;

	printf(" format3 subformat1");
	printf(" branch=%u", data->branch);
	printf(" privilege=0x%x", data->privilege);
	if (!params_g.itrace.notime_p)
		printf(" time=0x%x", data->time);
	if (!params_g.itrace.nocontext_p)
		printf(" context=0x%x", data->context);
	printf(" ecause=0x%x", data->ecause);
	printf(" interrupt=%u", data->interrupt);
	printf(" thaddr=%u", data->thaddr);
	printf(" address=0x%llx", data->address);
	if (!data->interrupt)
		printf(" tval=0x%llx", data->tval);
}

static void display_itrace_format30(unsigned int in_pkt_num,
				    const struct rv_itrace_data *it)
{
	const struct rv_itrace_format30 *data = &it->format3.format30;

	printf(" format3 subformat0");
	printf(" branch=%u", data->branch);
	printf(" privilege=0x%x", data->privilege);
	if (!params_g.itrace.notime_p)
		printf(" time=0x%x", data->time);
	if (!params_g.itrace.nocontext_p)
		printf(" context=0x%x", data->context);
	printf(" address=0x%llx", data->address);
}

int rv_etrace_pktdump(const unsigned char *packet_stream, size_t packet_stream_size,
		      size_t wp)
{
	unsigned int in_pkt_size, in_pkt_num = 0;
	const struct rv_etrace_packet *in_pkt;
	struct rv_etrace_payload in_pld;
	struct rv_itrace_data in_it;
	size_t pos, pos_orig, end;
	int rc, wrap = wp & 0x1;

	wp &= ~0x1;
	if (wrap) {
		pos = pos_orig = wp;
		end = packet_stream_size;
	} else {
		pos = 0;
		end = wp ? wp : packet_stream_size;
	}

	while (pos < end) {
		if (!packet_stream[pos]) {
			pos++;
			if (pos >= end && wrap) {
				pos -= end;
				end = pos_orig;
				wrap = 0;
			}
			continue;
		}
		in_pkt = (const struct rv_etrace_packet *)&packet_stream[pos];
		in_pkt_size = rv_etrace_packet_size(&params_g, in_pkt);
		if (wrap && (pos + in_pkt_size > end)) {
			struct rv_etrace_packet conc_pkt;
			int part_size = end - pos;
			memcpy((void *)&conc_pkt, &packet_stream[pos],
				part_size);
			memcpy((void *)&conc_pkt + part_size, &packet_stream[0],
				in_pkt_size - part_size);
			in_pkt  = &conc_pkt;
		}

		printf("%016lx: packet%08d:", pos, in_pkt_num);
		rc = rv_etrace_packet_payload_read(&params_g, in_pkt, &in_pld);
		if (rc < 0) {
			pos++;
			if (pos >= end && wrap) {
				pos -= end;
				end = pos_orig;
				wrap = 0;
			}
			continue;
		}

		switch (rv_etrace_payload_type_read(&params_g, &in_pld)) {
		case RV_ETRACE_PAYLOAD_TYPE_ITRACE:
			rc = rv_itrace_payload_read(&params_g, &in_pld, &in_it);
			if (rc) {
				printf(" error: failed to read itrace data (error %d)\n", rc);
				pos++;
				if (pos >= end && wrap) {
					pos -= end;
					end = pos_orig;
					wrap = 0;
				}
				continue;
			}

			switch (in_it.format) {
			case 1:
				display_itrace_format1(in_pkt_num, &in_it);
				break;
			case 2:
				display_itrace_format2(in_pkt_num, &in_it);
				break;
			case 3:
				switch (in_it.format3.subformat) {
				case 0:
					display_itrace_format30(in_pkt_num, &in_it);
					break;
				case 1:
					display_itrace_format31(in_pkt_num, &in_it);
					break;
				case 2:
					display_itrace_format32(in_pkt_num, &in_it);
					break;
				case 3:
					display_itrace_format33(in_pkt_num, &in_it);
					break;
				default:
					printf(" error: unknown itrace format3 subformat %d\n",
						in_it.format3.subformat);
					pos++;
					if (pos >= end && wrap) {
						pos -= end;
						end = pos_orig;
						wrap = 0;
					}
					continue;
				}
				break;
			default:
				printf(" error: unknown itrace format %d\n", in_it.format);
				pos++;
				if (pos >= end && wrap) {
					pos -= end;
					end = pos_orig;
					wrap = 0;
				}
				continue;
			}
			break;
		case RV_ETRACE_PAYLOAD_TYPE_DTRACE:
		case RV_ETRACE_PAYLOAD_TYPE_UNKNOWN:
			printf(" error: unknown packet type\n");
			pos++;
			if (pos >= end && wrap) {
				pos -= end;
				end = pos_orig;
				wrap = 0;
			}
			continue;
		}

		printf("\n");

		pos += in_pkt_size;
		if (pos >= end && wrap) {
			pos -= end;
			end = pos_orig;
			wrap = 0;
		}
		in_pkt_num++;
	}

	return 0;
}