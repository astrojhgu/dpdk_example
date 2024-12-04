#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <rte_ip.h>
#include <rte_ether.h>

static std::string show_hdr(rte_ether_hdr* eth_hdr, rte_ipv4_hdr* ip_hdr, rte_udp_hdr* udp_hdr){
    auto src_mac_addr=eth_hdr->src_addr.addr_bytes;
    auto dst_mac_addr=eth_hdr->dst_addr.addr_bytes;
    auto src_addr=ip_hdr->src_addr;
    auto dst_addr=ip_hdr->dst_addr;
    auto src_port=rte_be_to_cpu_16(udp_hdr->src_port);
    auto dst_port=rte_be_to_cpu_16(udp_hdr->dst_port);
    std::ostringstream oss;
    oss<<std::hex<<(uint16_t)src_mac_addr[0]<<":"<<(uint16_t)src_mac_addr[1]<<":"<<(uint16_t)src_mac_addr[2]
    <<":"<<(uint16_t)src_mac_addr[3]<<":"<<(uint16_t)src_mac_addr[4]<<":"<<(uint16_t)src_mac_addr[5]<<" "<<std::dec
    <<(src_addr&0xff)<<"."<<((src_addr>>8)&0xff)<<"."<<((src_addr>>16)&0xff)<<"."<<((src_addr>>24)&0xff)<<":"<<src_port;
    oss
    <<"->"
    <<std::hex<<(uint16_t)dst_mac_addr[0]<<":"<<(uint16_t)dst_mac_addr[1]<<":"<<(uint16_t)dst_mac_addr[2]
    <<":"<<(uint16_t)dst_mac_addr[3]<<":"<<(uint16_t)dst_mac_addr[4]<<":"<<(uint16_t)dst_mac_addr[5]<<" "<<std::dec
    <<(dst_addr&0xff)<<"."<<((dst_addr>>8)&0xff)<<"."<<((dst_addr>>16)&0xff)<<"."<<((dst_addr>>24)&0xff)<<":"<<dst_port;
    return oss.str();
}

#endif