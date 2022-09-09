#include <infiniband/mad.h>

#ifndef __PERFQUERY_H__
#define __PERFQUERY_H__

#define ALL_PORTS 0xFF
#define MAX_PORTS 255

#include <linux/types.h>

#define IB_PORT_EXT_RS_FEC2_MODE_ACTIVE (htobe16(0x0004))

typedef struct umad_port {
	char ca_name[20];
	int portnum;
	unsigned base_lid;
	unsigned lmc;
	unsigned sm_lid;
	unsigned sm_sl;
	unsigned state;
	unsigned phys_state;
	unsigned rate;
	__be32 capmask;
	__be64	 gid_prefix;
	__be64	 port_guid;
	unsigned pkeys_size;
	uint16_t *pkeys;
	char link_layer[20];
} umad_port_t;

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
} perf_data_t;

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