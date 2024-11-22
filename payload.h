#ifndef PAYLOAD_H
#define PAYLOAD_H
#include <cinttypes>
#include <rte_ip.h>
#include <rte_ether.h>
#include <rte_udp.h>
#include <rte_mbuf.h>
#include "config.h"

struct Payload{
    uint32_t head=0x12345678;//4 4
    uint32_t version=0xa1a1a1a1;//4 8
    uint64_t pkt_cnt=0;//8 16
    int64_t base_id=0;//8 24
    uint64_t port_id=0;//8 32
    uint64_t npt_per_frame=N_PT_PER_FRAME;//8 40
    uint64_t _reserve=0;//8 48
    int16_t data[N_PT_PER_FRAME]={};//8192 8240
};


void unpack_data (const rte_mbuf *buf, rte_ether_hdr **ether_hdr, rte_ipv4_hdr **ipv4_hdr, rte_udp_hdr **udp_hdr, Payload **payload);
void pack_data (rte_mbuf *buf, rte_ether_hdr *ether_hdr, rte_ipv4_hdr *ipv4_hdr, rte_udp_hdr *udp_hdr, Payload *payload);


static constexpr size_t payload_len(){
    return sizeof(Payload);
}

static constexpr size_t udp_pkt_len(){
    return sizeof(rte_udp_hdr)+payload_len();
}

static constexpr size_t ip_pkt_len(){
    return sizeof(rte_ipv4_hdr)+udp_pkt_len();
}

static constexpr size_t ether_pkt_len(){
    auto result=sizeof(rte_ether_hdr)+ip_pkt_len();
    assert(result<=MTU);
    return result;
}

#endif
