#include <stdio.h>

#include <mad.h>
#include <iba/ib_types.h>

#include <inttypes.h>
#include <linux/types.h>

#include "querrynclude.h"


extern uint8_t pc[1024];
extern struct ibmad_port *srcport;
static uint ibd_timeout = 20;

extern struct info_s info;

int is_port_info_extended_supported(ib_portid_t * dest, int port, struct ibmad_port *srcport)
{
        uint8_t data[IB_SMP_DATA_SIZE] = { 0 };
        uint32_t cap_mask;
        uint16_t cap_mask2;
        int type, portnum;

        if (!smp_query_via(data, dest, IB_ATTR_NODE_INFO, 0, 0, srcport))
                printf("node info query failed");

        mad_decode_field(data, IB_NODE_TYPE_F, &type);
        if (type == IB_NODE_SWITCH)
                portnum = 0;
        else
                portnum = port;

        if (!smp_query_via(data, dest, IB_ATTR_PORT_INFO, portnum, 0, srcport))
                printf("port info query failed");

        mad_decode_field(data, IB_PORT_CAPMASK_F, &cap_mask);
        if (cap_mask & be32toh(IB_PORT_CAP_HAS_CAP_MASK2)) {
                mad_decode_field(data, IB_PORT_CAPMASK2_F, &cap_mask2);
                if (!(cap_mask2 &
                      be16toh(IB_PORT_CAP2_IS_PORT_INFO_EXT_SUPPORTED))) {
                        printf("port info capability mask2 = 0x%x doesn't"
                               " indicate PortInfoExtended support", cap_mask2);
                        return 0;
                }
        } else {
                printf("port info capability mask2 not supported");
                return 0;
        }

        return 1;
}


static uint8_t is_rsfec_mode_active(ib_portid_t * portid, int port, __be16 cap_mask)
{
    uint8_t data[IB_SMP_DATA_SIZE] = { 0 };
    uint32_t fec_mode_active = 0;
    uint32_t pie_capmask = 0;
    if (cap_mask & IS_PM_RSFEC_COUNTERS_SUP) {
        if (!is_port_info_extended_supported(portid, port, srcport)) {
            printf("Port Info Extended not supported");
            return 0;
        }

        if (!smp_query_via(data, portid, IB_ATTR_PORT_INFO_EXT, port, 0,
                   srcport))
            printf("smp query portinfo extended failed");

        mad_decode_field(data, IB_PORT_EXT_CAPMASK_F, &pie_capmask);
        mad_decode_field(data, IB_PORT_EXT_FEC_MODE_ACTIVE_F,
                 &fec_mode_active);
        if((pie_capmask &
            be32toh(IB_PORT_EXT_CAP_IS_FEC_MODE_SUPPORTED)) &&
           ((be16toh(IB_PORT_EXT_RS_FEC_MODE_ACTIVE) == (fec_mode_active & 0xffff)) ||
                    (be16toh(IB_PORT_EXT_RS_FEC2_MODE_ACTIVE) == (fec_mode_active & 0xffff))))
            return 1;
    }

    return 0;
}

static void common_func(ib_portid_t * portid, int port_num, int mask,
unsigned query, unsigned reset, const char *name, uint16_t attr,
void dump_func(char *, int, void *, int))
{
    char buf[1536];

    if (query) {
        memset(pc, 0, sizeof(pc));
        if (!pma_query_via(pc, portid, port_num, ibd_timeout, attr,
                   srcport))
            printf("cannot query %s", name);

        dump_func(buf, sizeof(buf), pc, sizeof(pc));

        printf("# %s counters: %s port %d\n%s", name,
               portid2str(portid), port_num, buf);
    }

    memset(pc, 0, sizeof(pc));
    if (reset && !performance_reset_via(pc, portid, info.port, mask,
                        ibd_timeout, attr, srcport))
        printf("cannot reset %s", name);
}

void xmt_sl_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortXmitDataSL",
            IB_GSI_PORT_XMIT_DATA_SL, mad_dump_perfcounters_xmt_sl);
}

void rcv_sl_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortRcvDataSL",
            IB_GSI_PORT_RCV_DATA_SL, mad_dump_perfcounters_rcv_sl);
}

void xmt_disc_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortXmitDiscardDetails",
            IB_GSI_PORT_XMIT_DISCARD_DETAILS,
            mad_dump_perfcounters_xmt_disc);
}

void rcv_err_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortRcvErrorDetails",
            IB_GSI_PORT_RCV_ERROR_DETAILS,
            mad_dump_perfcounters_rcv_err);
}

static uint8_t *ext_speeds_reset_via(void *rcvbuf, ib_portid_t * dest,
int port, uint64_t mask, unsigned timeout)
{
    ib_rpc_t rpc = { 0 };
    int lid = dest->lid;

    printf("lid %u port %d mask 0x%" PRIx64, lid, port, mask);
    if (lid == -1) {
        printf("only lid routed is supported");
        return NULL;
    }

    if (!mask)
        mask = ~0;

    rpc.mgtclass = IB_PERFORMANCE_CLASS;
    rpc.method = IB_MAD_METHOD_SET;
    rpc.attr.id = IB_GSI_PORT_EXT_SPEEDS_COUNTERS;

    memset(rcvbuf, 0, IB_MAD_SIZE);

    mad_set_field(rcvbuf, 0, IB_PESC_PORT_SELECT_F, port);
    mad_set_field64(rcvbuf, 0, IB_PESC_COUNTER_SELECT_F, mask);
    rpc.attr.mod = 0;
    rpc.timeout = timeout;
    rpc.datasz = IB_PC_DATA_SZ;
    rpc.dataoffs = IB_PC_DATA_OFFS;
    if (!dest->qp)
        dest->qp = 1;
    if (!dest->qkey)
        dest->qkey = IB_DEFAULT_QP1_QKEY;

    return mad_rpc(srcport, &rpc, dest, rcvbuf, rcvbuf);
}

void extended_speeds_query(ib_portid_t * portid, int port,
uint64_t ext_mask, __be16 cap_mask)
{
    int mask = ext_mask;

    if (!info.reset_only) {
        if (is_rsfec_mode_active(portid, port, cap_mask))
            common_func(portid, port, mask, 1, 0,
                    "PortExtendedSpeedsCounters with RS-FEC Active",
                    IB_GSI_PORT_EXT_SPEEDS_COUNTERS,
                    mad_dump_port_ext_speeds_counters_rsfec_active);
        else
            common_func(portid, port, mask, 1, 0,
                "PortExtendedSpeedsCounters",
                IB_GSI_PORT_EXT_SPEEDS_COUNTERS,
                mad_dump_port_ext_speeds_counters);
    }

    if ((info.reset_only || info.reset) &&
        !ext_speeds_reset_via(pc, portid, port, ext_mask, ibd_timeout))
        printf("cannot reset PortExtendedSpeedsCounters");
}

void oprcvcounters_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortOpRcvCounters",
            IB_GSI_PORT_PORT_OP_RCV_COUNTERS,
            mad_dump_perfcounters_port_op_rcv_counters);
}

void flowctlcounters_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortFlowCtlCounters",
            IB_GSI_PORT_PORT_FLOW_CTL_COUNTERS,
            mad_dump_perfcounters_port_flow_ctl_counters);
}

void vloppackets_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortVLOpPackets",
            IB_GSI_PORT_PORT_VL_OP_PACKETS,
            mad_dump_perfcounters_port_vl_op_packet);
}

void vlopdata_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortVLOpData",
            IB_GSI_PORT_PORT_VL_OP_DATA,
            mad_dump_perfcounters_port_vl_op_data);
}

void vlxmitflowctlerrors_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset),
            "PortVLXmitFlowCtlUpdateErrors",
            IB_GSI_PORT_PORT_VL_XMIT_FLOW_CTL_UPDATE_ERRORS,
            mad_dump_perfcounters_port_vl_xmit_flow_ctl_update_errors);
}

void vlxmitcounters_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortVLXmitWaitCounters",
            IB_GSI_PORT_PORT_VL_XMIT_WAIT_COUNTERS,
            mad_dump_perfcounters_port_vl_xmit_wait_counters);
}

void swportvlcong_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "SwPortVLCongestion",
            IB_GSI_SW_PORT_VL_CONGESTION,
            mad_dump_perfcounters_sw_port_vl_congestion);
}

void rcvcc_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortRcvConCtrl",
            IB_GSI_PORT_RCV_CON_CTRL,
            mad_dump_perfcounters_rcv_con_ctrl);
}
void slrcvfecn_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortSLRcvFECN",
            IB_GSI_PORT_SL_RCV_FECN, mad_dump_perfcounters_sl_rcv_fecn);
}

void slrcvbecn_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortSLRcvBECN",
            IB_GSI_PORT_SL_RCV_BECN, mad_dump_perfcounters_sl_rcv_becn);
}

void xmitcc_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortXmitConCtrl",
            IB_GSI_PORT_XMIT_CON_CTRL,
            mad_dump_perfcounters_xmit_con_ctrl);
}

void vlxmittimecc_query(ib_portid_t * portid, int port, int mask)
{
    common_func(portid, port, mask, !info.reset_only,
            (info.reset_only || info.reset), "PortVLXmitTimeCong",
            IB_GSI_PORT_VL_XMIT_TIME_CONG,
            mad_dump_perfcounters_vl_xmit_time_cong);
}

