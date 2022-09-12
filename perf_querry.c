#include <stdio.h>
#include <mad.h>
#include <umad.h>
#include <inttypes.h>
#include <string.h>

#include "querrynclude.h"

static uint8_t pc[1024];
uint ibd_timeout = 20;
struct ibmad_port *srcport;
struct info_s info;


// ERR_QUERRY
//______________________________________________________________________________

// static int _dump_fields(char *buf, int bufsz, void *data, int start, int end)
// {
// 	char val[64];
// 	char *s = buf;
// 	int n, field;

// 	for (field = start; field < end && bufsz > 0; field++) {
// 		mad_decode_field(data, field, val);
// 		if (!mad_dump_field(field, s, bufsz-1, val))
// 			return -1;
// 		n = strlen(s);
// 		s += n;
// 		*s++ = '\n';
// 		*s = 0;
// 		n++;
// 		bufsz -= n;
// 	}
// 	return (int)(s - buf);
// }

// static char *rcv_err_query(ib_portid_t * portid, int port, int mask)
// {
//     char buf[1536];
//     int cnt = 0;
 
//     memset(pc, 0, sizeof(pc));
//     if (!pma_query_via(pc, portid, port, ibd_timeout, IB_GSI_PORT_RCV_ERROR_DETAILS, srcport))
//         return (NULL);
//     memset(pc, 0, sizeof(pc));
//     mad_dump_perfcounters_rcv_err(buf, sizeof(buf), pc, sizeof(pc));
//     cnt = _dump_fields(buf, sizeof(buf), pc, IB_PC_EXT_PORT_SELECT_F, IB_PC_EXT_XMT_BYTES_F);
//     if (cnt < 0)
// 		return (NULL);
//     _dump_fields(buf + cnt, sizeof(buf) - cnt, pc, IB_PC_RCV_LOCAL_PHY_ERR_F, IB_PC_RCV_ERR_LAST_F);
//     if ((info.reset_only || info.reset) &&
//     !performance_reset_via(pc, portid, info.port, mask, ibd_timeout, IB_GSI_PORT_RCV_ERROR_DETAILS, srcport))
//         return (NULL);
//     return (strdup(buf));
// }

// static void print_err(void)
// {
//     printf("err get_err_querry");
// }

// static void get_err_query(perf_data_t *perf_count, ib_portid_t * portid, int port, int mask)
// {
//     char *buf = NULL;
//     char *tmp = NULL;
 
//     buf = rcv_err_query(portid, port, mask);
//     if (buf == NULL)
//         return (print_err());
//     tmp = buf;
//     tmp = strstr(tmp, "PortLocalPhysicalErrors");
//     if (tmp == NULL)
//         return (print_err());
//     for (tmp += 25; tmp[0] == '.' && tmp[0] != 0; tmp++);
//     perf_count->portlocalphysicalerrors = strtoul(tmp, NULL, 10);
//     tmp = strstr(tmp, "PortMalformedPktErrors");
//     if (tmp == NULL)
//         return (print_err());
//     for (tmp += 25; tmp[0] == '.' && tmp[0] != 0; tmp++);
//     perf_count->portmalformedpkterrors = strtoul(tmp, NULL, 10);
//     tmp = strstr(tmp, "PortBufferOverrunErrors");
//     if (tmp == NULL)
//         return (print_err());
//     for (tmp += 25; tmp[0] == '.' && tmp[0] != 0; tmp++);
//     perf_count->portbufferoverrunerrors = strtoul(tmp, NULL, 10);
//     tmp = strstr(tmp, "PortDLIDMappingErrors");
//     if (tmp == NULL)
//         return (print_err());
//     for (tmp += 23; tmp[0] == '.' && tmp[0] != 0; tmp++);
//     perf_count->portdlidmappingerrors = strtoul(tmp, NULL, 10);
//     tmp = strstr(tmp, "PortVLMappingErrors");
//     if (tmp == NULL)
//         return (print_err());
//     for (tmp += 21; tmp[0] == '.' && tmp[0] != 0; tmp++);
//     perf_count->portvlmappingerrors = strtoul(tmp, NULL, 10);
//     tmp = strstr(tmp, "PortLoopingErrors");
//     if (tmp == NULL)
//         return (print_err());
//     for (tmp += 19; tmp[0] == '.' && tmp[0] != 0; tmp++);
//     perf_count->portloopingerrors = strtoul(tmp, NULL, 10);
//     if (buf != NULL)
//         free(buf);
//     static u_int32_t val;
    
// }

// AGGREGATE TOOLS
//______________________________________________________________________________

static void aggregate_4bit(uint32_t * dest, uint32_t val)
{
    if ((((*dest) + val) < (*dest)) || ((*dest) + val) > 0xf)
        (*dest) = 0xf;
    else
        (*dest) = (*dest) + val;
}

static void aggregate_8bit(uint32_t * dest, uint32_t val)
{
    if ((((*dest) + val) < (*dest))
        || ((*dest) + val) > 0xff)
        (*dest) = 0xff;
    else
        (*dest) = (*dest) + val;
}

static void aggregate_16bit(uint32_t * dest, uint32_t val)
{
    if ((((*dest) + val) < (*dest))
        || ((*dest) + val) > 0xffff)
        (*dest) = 0xffff;
    else
        (*dest) = (*dest) + val;
}

static void aggregate_32bit(uint32_t * dest, uint32_t val)
{
    if (((*dest) + val) < (*dest))
        (*dest) = 0xffffffff;
    else
        (*dest) = (*dest) + val;
}

// MAIN AGGREGATOR
//______________________________________________________________________________

static void aggregate_perfcounters(perf_data_t *perf_count)
{
    static uint32_t val;

    mad_decode_field(pc, IB_PC_PORT_SELECT_F, &val);
    perf_count->portselect = val;
    mad_decode_field(pc, IB_PC_COUNTER_SELECT_F, &val);
    perf_count->counterselect = val;
    mad_decode_field(pc, IB_PC_ERR_SYM_F, &val);
    aggregate_16bit(&perf_count->symbolerrors, val);
    mad_decode_field(pc, IB_PC_LINK_RECOVERS_F, &val);
    aggregate_8bit(&perf_count->linkrecovers, val);
    mad_decode_field(pc, IB_PC_LINK_DOWNED_F, &val);
    aggregate_8bit(&perf_count->linkdowned, val);
    mad_decode_field(pc, IB_PC_ERR_RCV_F, &val);
    aggregate_16bit(&perf_count->rcverrors, val);
    mad_decode_field(pc, IB_PC_ERR_PHYSRCV_F, &val);
    aggregate_16bit(&perf_count->rcvremotephyerrors, val);
    mad_decode_field(pc, IB_PC_ERR_SWITCH_REL_F, &val);
    aggregate_16bit(&perf_count->rcvswrelayerrors, val);
    mad_decode_field(pc, IB_PC_XMT_DISCARDS_F, &val);
    aggregate_16bit(&perf_count->xmtdiscards, val);
    mad_decode_field(pc, IB_PC_ERR_XMTCONSTR_F, &val);
    aggregate_8bit(&perf_count->xmtconstrainterrors, val);
    mad_decode_field(pc, IB_PC_ERR_RCVCONSTR_F, &val);
    aggregate_8bit(&perf_count->rcvconstrainterrors, val);
    mad_decode_field(pc, IB_PC_ERR_LOCALINTEG_F, &val);
    aggregate_4bit(&perf_count->linkintegrityerrors, val);
    mad_decode_field(pc, IB_PC_ERR_EXCESS_OVR_F, &val);
    aggregate_4bit(&perf_count->excbufoverrunerrors, val);
    mad_decode_field(pc, IB_PC_QP1_DROP_F, &val);
    aggregate_16bit(&perf_count->qp1dropped, val);
    mad_decode_field(pc, IB_PC_VL15_DROPPED_F, &val);
    aggregate_16bit(&perf_count->vl15dropped, val);
    mad_decode_field(pc, IB_PC_XMT_BYTES_F, &val);
    aggregate_32bit(&perf_count->xmtdata, val);
    mad_decode_field(pc, IB_PC_RCV_BYTES_F, &val);
    aggregate_32bit(&perf_count->rcvdata, val);
    mad_decode_field(pc, IB_PC_XMT_PKTS_F, &val);
    aggregate_32bit(&perf_count->xmtpkts, val);
    mad_decode_field(pc, IB_PC_RCV_PKTS_F, &val);
    aggregate_32bit(&perf_count->rcvpkts, val);
    mad_decode_field(pc, IB_PC_XMT_WAIT_F, &val);
    aggregate_32bit(&perf_count->xmtwait, val);
    mad_decode_field(pc, IB_PC_RCV_LOCAL_PHY_ERR_F, &val);
    aggregate_32bit(&perf_count->portlocalphysicalerrors, val);
    mad_decode_field(pc, IB_PC_RCV_MALFORMED_PKT_ERR_F, &val);
    aggregate_32bit(&perf_count->portmalformedpkterrors, val);
    mad_decode_field(pc, IB_PC_RCV_BUF_OVR_ERR_F, &val);
    aggregate_32bit(&perf_count->portbufferoverrunerrors, val);
    mad_decode_field(pc, IB_PC_RCV_DLID_MAP_ERR_F, &val);
    aggregate_32bit(&perf_count->portdlidmappingerrors, val);
    mad_decode_field(pc, IB_PC_RCV_VL_MAP_ERR_F, &val);
    aggregate_32bit(&perf_count->portvlmappingerrors, val);
    mad_decode_field(pc, IB_PC_RCV_LOOPING_ERR_F, &val);
    aggregate_32bit(&perf_count->portloopingerrors, val);
}

// DUMPER
//______________________________________________________________________________

static void dump_perfcounters(int extended, int timeout, __be16 cap_mask, uint32_t cap_mask2,
ib_portid_t * portid, int port, int aggregate, perf_data_t *perf_count)
{
	memset(pc, 0, sizeof(pc));
	if (!pma_query_via(pc, portid, port, timeout, IB_GSI_PORT_COUNTERS, srcport)) {
		printf("perfquery");
        return;
    }
	aggregate_perfcounters(perf_count);
}

// RESOLVE SELF
//______________________________________________________________________________

static int resolve_self(char *ca_name, uint8_t ca_port, ib_portid_t *portid,
		 int *portnum, ibmad_gid_t *gid)
{
	umad_port_t port;
	int rc;

	if (!(portid || portnum || gid))
		return (21);
	if ((rc = umad_get_port(ca_name, ca_port, &port)) < 0)
		return rc;
	if (portid) {
		memset(portid, 0, sizeof(*portid));
		portid->lid = port.base_lid;
		portid->sl = port.sm_sl;
	}
	if (portnum)
		*portnum = port.portnum;
	umad_release_port(&port);
	return (0);
}

// MAIN
//______________________________________________________________________________

int main(int ac, char **av)
{
    perf_data_t *perf_count = NULL;
    char *ibd_ca = NULL;
    int ibd_ca_port = 1;
    int mgmt_classes[3] = { IB_SMI_CLASS, IB_SA_CLASS, IB_PERFORMANCE_CLASS };
    ib_portid_t portid;
    int mask = 0xffff;

    perf_count = malloc(sizeof(perf_data_t));
    if (ac > 1)
        ibd_ca = av[1];
    if (resolve_self(ibd_ca, ibd_ca_port, &portid, &info.port, NULL) < 0)
        return (42);
    srcport = mad_rpc_open_port(ibd_ca, ibd_ca_port, mgmt_classes, 3);
    if (!srcport)
        return (21);
    dump_perfcounters(0, ibd_timeout, mask, 0, &portid, 1, 1, perf_count);
    // get_err_query(perf_count ,&portid, ibd_ca_port, mask);
    mad_rpc_close_port(srcport);
    printf("portlocalphysicalerrors: %lu\n", perf_count->portlocalphysicalerrors);
    free (perf_count);
    return (0);
}
