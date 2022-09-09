#include <stdio.h>
#include <mad.h>
#include <umad.h>
#include <inttypes.h>

#include "querrynclude.h"

static uint8_t pc[1024];
uint ibd_timeout = 20;
struct ibmad_port *srcport;
struct info_s info;

static int resolve_self(char *ca_name, uint8_t ca_port, ib_portid_t *portid,
		 int *portnum, ibmad_gid_t *gid)
{
    printf ("resolve_self");
	umad_port_t port;
	uint64_t prefix, guid;
	int rc;

	if (!(portid || portnum || gid))
		return (-1);
    printf("resolve_self2");
	if ((rc = umad_get_port(ca_name, ca_port, &port)) < 0)
		return rc;
    printf("resolve_self3");
	if (portid) {
		memset(portid, 0, sizeof(*portid));
		portid->lid = port.base_lid;
		portid->sl = port.sm_sl;
	}
    printf("resolve_self4");
	if (portnum)
		*portnum = port.portnum;
    printf("resolve_self5");
	if (gid) {
		memset(gid, 0, sizeof(*gid));
		prefix = be64toh(port.gid_prefix);
		guid = be64toh(port.port_guid);
		mad_encode_field(*gid, IB_GID_PREFIX_F, &prefix);
		mad_encode_field(*gid, IB_GID_GUID_F, &guid);
	}
    printf("resolve_self6");
	umad_release_port(&port);
    printf("resolve_self7");
	return 0;
}

void aggregate_4bit(uint32_t * dest, uint32_t val)
{
    if ((((*dest) + val) < (*dest)) || ((*dest) + val) > 0xf)
        (*dest) = 0xf;
    else
        (*dest) = (*dest) + val;
}

void aggregate_8bit(uint32_t * dest, uint32_t val)
{
    if ((((*dest) + val) < (*dest))
        || ((*dest) + val) > 0xff)
        (*dest) = 0xff;
    else
        (*dest) = (*dest) + val;
}

void aggregate_16bit(uint32_t * dest, uint32_t val)
{
    if ((((*dest) + val) < (*dest))
        || ((*dest) + val) > 0xffff)
        (*dest) = 0xffff;
    else
        (*dest) = (*dest) + val;
}

void aggregate_32bit(uint32_t * dest, uint32_t val)
{
    if (((*dest) + val) < (*dest))
        (*dest) = 0xffffffff;
    else
        (*dest) = (*dest) + val;
}

void aggregate_64bit(uint64_t * dest, uint64_t val)
{
    if (((*dest) + val) < (*dest))
        (*dest) = 0xffffffffffffffffULL;
    else
        (*dest) = (*dest) + val;
}

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
}


static void dump_perfcounters(int extended, int timeout, __be16 cap_mask, uint32_t cap_mask2,
ib_portid_t * portid, int port, int aggregate, perf_data_t *perf_count)
{
	char buf[1536];

	memset(pc, 0, sizeof(pc));
	if (!pma_query_via(pc, portid, port, timeout, IB_GSI_PORT_COUNTERS, srcport)) {
		printf("perfquery");
        return;
    }
	if (aggregate)
		aggregate_perfcounters(perf_count);
	else
		mad_dump_perfcounters(buf, sizeof buf, pc, sizeof pc);
}

int main(int ac, char **av)
{
    perf_data_t *perf_count = NULL;
    char *ibd_ca = "mlx5_2";
    int ibd_ca_port = 1;
    int mgmt_classes[3] = { IB_SMI_CLASS, IB_SA_CLASS, IB_PERFORMANCE_CLASS };
    ib_portid_t portid = { 1 };
    int mask = 0xffff;

    perf_count = malloc(sizeof(perf_data_t));
    if (ac > 1)
        ibd_ca = av[1];
    resolve_self(ibd_ca, ibd_ca_port, &portid, &info.port, NULL);
    srcport = mad_rpc_open_port(ibd_ca, ibd_ca_port, mgmt_classes, 3);
    if (!srcport) {
        printf("Failed to open '%s' port '%d'\n", ibd_ca, ibd_ca_port);
        return (-1);
    }
    if (!smp_query_via(pc, &portid, IB_ATTR_SWITCH_INFO, 0, ibd_timeout, srcport))
        return -1;
    dump_perfcounters(0, ibd_timeout, mask, 0, &portid, 1, 1, perf_count);
    printf("port: %u\nsymbolerrors: %u\nPortXmitDiscards: %u\n",
    perf_count->portselect, perf_count->symbolerrors, perf_count->xmtdiscards);
    free (perf_count);
    mad_rpc_close_port(srcport);
    return (0);
}