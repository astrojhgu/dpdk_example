#ifndef PAYLOAD_H
#define PAYLOAD_H
#include <cinttypes>
#include <rte_ip.h>
#include <rte_ether.h>
#include <rte_udp.h>
#include <rte_mbuf.h>
#include "config.h"

struct Payload{
    uint64_t pkt_cnt;//8 8
    int64_t base_id;//8 16
    uint64_t port_id;//4 20
    uint64_t _reserve;
    int16_t data[N_PT_PER_FRAME];//8192 8216
};


void unpack_data (const rte_mbuf *buf, rte_ether_hdr **ether_hdr, rte_ipv4_hdr **ipv4_hdr, rte_udp_hdr **udp_hdr, Payload **payload);
void pack_data (rte_mbuf *buf, rte_ether_hdr *ether_hdr, rte_ipv4_hdr *ipv4_hdr, rte_udp_hdr *udp_hdr, Payload *payload);

static constexpr size_t pkt_len(){
    return sizeof(rte_ether_hdr)+sizeof(rte_ipv4_hdr)+sizeof(rte_udp_hdr)+sizeof(Payload);
}

#endif
