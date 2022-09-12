#include <stdio.h>
#include <mad.h>
#include <umad.h>
#include <inttypes.h>
#include <string.h>

#include "querrynclude.h"

uint ibd_timeout = 20;
struct ibmad_port *srcport;
struct info_s info;

// MAIN AGGREGATOR
//______________________________________________________________________________

static void aggregate_perfcounters(perf_data_t *perf_count, uint8_t pc[])
{
    static uint32_t val;

    mad_decode_field(pc, IB_PC_EXT_PORT_SELECT_F, &val);
    perf_count->portselect = val;
    mad_decode_field(pc, IB_PC_COUNTER_SELECT_F, &val);
    perf_count->counterselect = val;
    mad_decode_field(pc, IB_PC_ERR_SYM_F, &val);
    perf_count->symbolerrors = val;
    mad_decode_field(pc, IB_PC_LINK_RECOVERS_F, &val);
    perf_count->linkrecovers = val;
    mad_decode_field(pc, IB_PC_LINK_DOWNED_F, &val);
    perf_count->linkdowned = val;
    mad_decode_field(pc, IB_PC_ERR_RCV_F, &val);
    perf_count->rcverrors = val;
    mad_decode_field(pc, IB_PC_ERR_PHYSRCV_F, &val);
    perf_count->rcvremotephyerrors = val;
    mad_decode_field(pc, IB_PC_ERR_SWITCH_REL_F, &val);
    perf_count->rcvswrelayerrors = val;
    mad_decode_field(pc, IB_PC_XMT_DISCARDS_F, &val);
    perf_count->xmtdiscards = val;
    mad_decode_field(pc, IB_PC_ERR_XMTCONSTR_F, &val);
    perf_count->xmtconstrainterrors = val;
    mad_decode_field(pc, IB_PC_ERR_RCVCONSTR_F, &val);
    perf_count->rcvconstrainterrors = val;
    mad_decode_field(pc, IB_PC_ERR_LOCALINTEG_F, &val);
    perf_count->linkintegrityerrors = val;
    mad_decode_field(pc, IB_PC_ERR_EXCESS_OVR_F, &val);
    perf_count->excbufoverrunerrors = val;
    mad_decode_field(pc, IB_PC_QP1_DROP_F, &val);
    perf_count->qp1dropped = val;
    mad_decode_field(pc, IB_PC_VL15_DROPPED_F, &val);
    perf_count->vl15dropped = val;
    mad_decode_field(pc, IB_PC_XMT_BYTES_F, &val);
    perf_count->xmtdata = val;
    mad_decode_field(pc, IB_PC_RCV_BYTES_F, &val);
    perf_count->rcvdata = val;
    mad_decode_field(pc, IB_PC_XMT_PKTS_F, &val);
    perf_count->xmtpkts = val;
    mad_decode_field(pc, IB_PC_RCV_PKTS_F, &val);
    perf_count->rcvpkts = val;
    mad_decode_field(pc, IB_PC_XMT_WAIT_F, &val);
    perf_count->xmtwait = val;
}

static void aggregate_ext_perfcounters(perf_data_t *perf_count, uint8_t pc[])
{
    static u_int32_t val;

    mad_decode_field(pc, IB_PC_RCV_LOCAL_PHY_ERR_F, &val);
    perf_count->portlocalphysicalerrors = val;
    mad_decode_field(pc, IB_PC_RCV_MALFORMED_PKT_ERR_F, &val);
    perf_count->portmalformedpkterrors = val;
    mad_decode_field(pc, IB_PC_RCV_BUF_OVR_ERR_F, &val);
    perf_count->portbufferoverrunerrors = val;
    mad_decode_field(pc, IB_PC_RCV_DLID_MAP_ERR_F, &val);
    perf_count->portdlidmappingerrors = val;
    mad_decode_field(pc, IB_PC_RCV_VL_MAP_ERR_F, &val);
    perf_count->portvlmappingerrors = val;
    mad_decode_field(pc, IB_PC_RCV_LOOPING_ERR_F, &val);
    perf_count->portloopingerrors = val;
}

// DUMPER
//______________________________________________________________________________

static void dump_perfcounters(ib_portid_t * portid, int port, perf_data_t *perf_count)
{
    static uint8_t pc[1024];

    memset(pc, 0, sizeof(pc));
	if (!pma_query_via(pc, portid, port, ibd_timeout, IB_GSI_PORT_COUNTERS, srcport)) {
		printf("perfquery");
        return;
    }
	aggregate_perfcounters(perf_count, pc);
    memset(pc, 0, sizeof(pc));
    if (!pma_query_via(pc, portid, port, ibd_timeout, IB_GSI_PORT_RCV_ERROR_DETAILS, srcport)){
		printf("extperfquery");
        return;
    }
    aggregate_ext_perfcounters(perf_count, pc);
}

// RESOLVE SELF
//______________________________________________________________________________

static int resolve_self(char *ca_name, uint8_t ca_port, ib_portid_t *portid, int *portnum)
{
	umad_port_t port;
	int rc;

	if (!(portid || portnum))
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
    if (resolve_self(ibd_ca, ibd_ca_port, &portid, &info.port) < 0)
        return (42);
    srcport = mad_rpc_open_port(ibd_ca, ibd_ca_port, mgmt_classes, 3);
    if (!srcport)
        return (21);
    perf_count->portlocalphysicalerrors = 3600;
    dump_perfcounters(&portid, 1, perf_count);
    mad_rpc_close_port(srcport);
    free (perf_count);
    return (0);
}
