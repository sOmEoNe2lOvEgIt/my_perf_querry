#include <stdio.h>
#include <mad.h>
#include <inttypes.h>

#include "querrynclude.h"

static uint8_t pc[1024];
uint ibd_timeout = 20;
struct ibmad_port *srcport;
struct info_s info;

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

int main(int ac, char **av)
{
    perf_data_t *perf_count = NULL;
    char *ibd_ca = NULL;
    int ibd_ca_port = 1;
    int mgmt_classes[3] = { IB_SMI_CLASS, IB_SA_CLASS, IB_PERFORMANCE_CLASS };
    ib_portid_t portid = { 1 };
    int mask = 0xffff;

    perf_count = malloc(sizeof(perf_data_t));
    if (ac > 1)
        ibd_ca = av[1];
    srcport = mad_rpc_open_port(ibd_ca, ibd_ca_port, mgmt_classes, 3);
    if (!srcport) {
        printf("Failed to open '%s' port '%d'\n", ibd_ca, ibd_ca_port);
        return (-1);
    }
    if (!pma_query_via(pc, &portid, info.port, ibd_timeout, CLASS_PORT_INFO, srcport))
        return -1;
    if (!smp_query_via(pc, &portid, info.port, ibd_timeout, CLASS_PORT_INFO, srcport))
        return -1;

    // xmt_sl_query(&portid, ibd_ca_port, mask);
    // rcv_sl_query(&portid, ibd_ca_port, mask);
    // xmt_disc_query(&portid, ibd_ca_port, mask);
    // rcv_err_query(&portid, ibd_ca_port, mask);
    
    perf_count = malloc(sizeof(perf_data_t));
    if (perf_count == NULL)
        return (1);
    aggregate_perfcounters(perf_count);
    printf("port: %u\nsymbolerrors: %u\n",
    perf_count->portselect, perf_count->symbolerrors);
    free (perf_count);
    mad_rpc_close_port(srcport);
    return (0);
}