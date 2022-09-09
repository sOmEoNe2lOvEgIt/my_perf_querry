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

void xmt_sl_query(ib_portid_t * portid, int port, int mask);

void rcv_sl_query(ib_portid_t * portid, int port, int mask);

void xmt_disc_query(ib_portid_t * portid, int port, int mask);

void rcv_err_query(ib_portid_t * portid, int port, int mask);

void extended_speeds_query(ib_portid_t * portid, int port, uint64_t ext_mask, __be16 cap_mask);

void oprcvcounters_query(ib_portid_t * portid, int port, int mask);

void flowctlcounters_query(ib_portid_t * portid, int port, int mask);

void vloppackets_query(ib_portid_t * portid, int port, int mask);

void vlopdata_query(ib_portid_t * portid, int port, int mask);

void vlxmitflowctlerrors_query(ib_portid_t * portid, int port, int mask);

void vlxmitcounters_query(ib_portid_t * portid, int port, int mask);

void swportvlcong_query(ib_portid_t * portid, int port, int mask);

void rcvcc_query(ib_portid_t * portid, int port, int mask);

void slrcvfecn_query(ib_portid_t * portid, int port, int mask);

void slrcvbecn_query(ib_portid_t * portid, int port, int mask);

void xmitcc_query(ib_portid_t * portid, int port, int mask);

void vlxmittimecc_query(ib_portid_t * portid, int port, int mask);
#endif /* __PERFQUERY_H__ */