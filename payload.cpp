#include "payload.h"

void unpack_data (const rte_mbuf *buf, rte_ether_hdr **ether_hdr, rte_ipv4_hdr **ipv4_hdr, rte_udp_hdr **udp_hdr, Payload **payload)
{
    char *ptr = rte_pktmbuf_mtod (buf, char *);
    *ether_hdr = (rte_ether_hdr *)ptr;
    *ipv4_hdr = (rte_ipv4_hdr *)(ptr + sizeof (rte_ether_hdr));
    *udp_hdr = (rte_udp_hdr *)(ptr + sizeof (rte_ether_hdr) + sizeof (rte_ipv4_hdr));
    *payload = (Payload *)(ptr + sizeof (rte_ether_hdr) + sizeof (rte_ipv4_hdr) + sizeof (rte_udp_hdr));
}


void pack_data (rte_mbuf *buf, rte_ether_hdr *ether_hdr, rte_ipv4_hdr *ipv4_hdr, rte_udp_hdr *udp_hdr, Payload *payload)
{
    char *ptr = rte_pktmbuf_mtod (buf, char *);
    if (ether_hdr) {
        *(rte_ether_hdr *)ptr = *ether_hdr;
    }
    if (ipv4_hdr) {
        *(rte_ipv4_hdr *)(ptr + sizeof (rte_ether_hdr)) = *ipv4_hdr;
    }
    if (udp_hdr) {
        *(rte_udp_hdr *)(ptr + sizeof (rte_ether_hdr) + sizeof (rte_ipv4_hdr)) = *udp_hdr;
    }
    *(Payload *)(ptr + sizeof (rte_ether_hdr) + sizeof (rte_ipv4_hdr) + sizeof (rte_udp_hdr)) = *payload;
}
