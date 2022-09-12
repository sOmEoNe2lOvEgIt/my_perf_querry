#include <infiniband/mad.h>

#ifndef __PERFQUERY_H__
#define __PERFQUERY_H__

#define ALL_PORTS 0xFF
#define MAX_PORTS 255

#include <linux/types.h>

#define IB_PORT_EXT_RS_FEC2_MODE_ACTIVE (htobe16(0x0004))

struct info_s
{
    int reset, reset_only, all_ports, loop_ports, port, extended, xmt_sl,
        rcv_sl, xmt_disc, rcv_err, extended_speeds, smpl_ctl,
        oprcvcounters, flowctlcounters, vloppackets, vlopdata,
        vlxmitflowctlerrors, vlxmitcounters, swportvlcong, rcvcc,
        slrcvfecn, slrcvbecn, xmitcc, vlxmittimecc;
    int ports[MAX_PORTS];
    int ports_count;
};

typedef struct perf_count {
    uint32_t portselect;
    uint32_t counterselect;
    uint32_t symbolerrors;
    uint32_t linkrecovers;
    uint32_t linkdowned;
    uint32_t rcverrors;
    uint32_t rcvremotephyerrors;
    uint32_t rcvswrelayerrors;
    uint32_t xmtdiscards;
    uint32_t xmtconstrainterrors;
    uint32_t rcvconstrainterrors;
    uint32_t linkintegrityerrors;
    uint32_t excbufoverrunerrors;
    uint32_t qp1dropped;
    uint32_t vl15dropped;
    uint32_t xmtdata;
    uint32_t rcvdata;
    uint32_t xmtpkts;
    uint32_t rcvpkts;
    uint32_t xmtwait;
} perf_data_t; // THAT WILL BE THE RETURN VALUE OF GATHERING FUNCTION PAIRED WHITH THE EXT DATA STRUCT. 

#endif /* __PERFQUERY_H__ */