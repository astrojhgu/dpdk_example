#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <rte_ip.h>
static std::string show_hdr(rte_ipv4_hdr* ip_hdr, rte_udp_hdr* udp_hdr){
    auto src_addr=ip_hdr->src_addr;
    auto dst_addr=ip_hdr->dst_addr;
    auto src_port=rte_be_to_cpu_16(udp_hdr->src_port);
    auto dst_port=rte_be_to_cpu_16(udp_hdr->dst_port);
    std::ostringstream oss;
    oss<<(src_addr&0xff)<<"."<<((src_addr>>8)&0xff)<<"."<<((src_addr>>16)&0xff)<<"."<<((src_addr>>24)&0xff)<<":"<<src_port;
    oss<<"->"<<(dst_addr&0xff)<<"."<<((dst_addr>>8)&0xff)<<"."<<((dst_addr>>16)&0xff)<<"."<<((dst_addr>>24)&0xff)<<":"<<dst_port;
    return oss.str();
}

#endif