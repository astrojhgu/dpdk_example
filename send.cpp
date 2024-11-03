/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include "config.h"

#include "yaml-cpp/yaml.h"

#include "payload.h"
#include <rte_mbuf_core.h>
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32


struct SendCfg
{
    int32_t dst_ip[4];
    int16_t dst_port;
    int32_t src_ip[4];
    int16_t src_port;
    uint8_t dst_mac[6];

    SendCfg (const YAML::Node &input)
    {
        auto dst_ip1 = input["dst_ip"].as<std::vector<int32_t>> ();
        auto src_ip1 = input["src_ip"].as<std::vector<int32_t>> ();
        for (int i = 0; i < 4; ++i) {
            dst_ip[i] = dst_ip1[i];
            src_ip[i] = src_ip1[i];
        }
        dst_port = input["dst_port"].as<int16_t> ();
        src_port = input["src_port"].as<int16_t> ();
        auto dst_mac1 = input["dst_mac"].as<std::vector<uint8_t>> ();
        for (int i = 0; i < 6; ++i) {
            dst_mac[i] = dst_mac1[i];
        }
    }
};

/* Main functional part of port initialization. 8< */
static inline int port_init (uint16_t port, struct rte_mempool *mbuf_pool)
{
    struct rte_eth_conf port_conf;
    const uint16_t rx_rings = 1, tx_rings = 1;
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    int retval;
    uint16_t q;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf txconf;

    if (!rte_eth_dev_is_valid_port (port)) return -1;

    memset (&port_conf, 0, sizeof (struct rte_eth_conf));

    retval = rte_eth_dev_info_get (port, &dev_info);
    if (retval != 0) {
        printf ("Error during getting device (port %u) info: %s\n", port, strerror (-retval));
        return retval;
    }

    /*
    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE & RTE_ETH_TX_OFFLOAD_IPV4_CKSUM & RTE_ETH_TX_OFFLOAD_UDP_CKSUM)
    {
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE | RTE_ETH_TX_OFFLOAD_IPV4_CKSUM | RTE_ETH_TX_OFFLOAD_UDP_CKSUM;
    }else{
        rte_panic(" offload not supported");
    }*/
    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) {
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
    }

    port_conf.rxmode.mtu = MTU;

    /* Configure the Ethernet device. */
    retval = rte_eth_dev_configure (port, rx_rings, tx_rings, &port_conf);
    if (retval != 0) return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc (port, &nb_rxd, &nb_txd);
    if (retval != 0) return retval;

    /* Allocate and set up 1 RX queue per Ethernet port. */
    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup (port, q, nb_rxd, rte_eth_dev_socket_id (port), NULL, mbuf_pool);
        if (retval < 0) return retval;
    }

    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;
    /* Allocate and set up 1 TX queue per Ethernet port. */
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup (port, q, nb_txd, rte_eth_dev_socket_id (port), &txconf);
        if (retval < 0) return retval;
    }

    /* Starting Ethernet port. 8< */
    retval = rte_eth_dev_start (port);
    /* >8 End of starting of ethernet port. */
    if (retval < 0) return retval;

    /* Display the port MAC address. */
    struct rte_ether_addr addr;
    retval = rte_eth_macaddr_get (port, &addr);
    if (retval != 0) return retval;

    printf ("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8
            " %02" PRIx8 "\n",
            port, RTE_ETHER_ADDR_BYTES (&addr));

    /* Enable RX in promiscuous mode for the Ethernet device. */
    retval = rte_eth_promiscuous_enable (port);
    /* End of setting RX port in promiscuous mode. */
    if (retval != 0) return retval;

    return 0;
}
/* >8 End of main functional part of port initialization. */

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */

/* Basic forwarding application lcore. 8< */
static void lcore_main (rte_mempool *mbuf_pool, SendCfg send_cfg)
{
    uint16_t port = 0;

    /*
     * Check that the port is on the same NUMA node as the polling thread
     * for best performance.
     */

    if (rte_eth_dev_socket_id (port) >= 0 && rte_eth_dev_socket_id (port) != (int)rte_socket_id ()) {
        printf ("WARNING, port %u is on remote NUMA node to "
                "polling thread.\n\tPerformance will "
                "not be optimal.\n",
                port);
    }

    printf ("\nCore %u forwarding packets. [Ctrl+C to quit]\n", rte_lcore_id ());


    rte_ether_hdr ether_hdr;
    rte_ipv4_hdr ipv4_hdr;
    rte_udp_hdr udp_hdr;
    Payload payload;

    for (int i = 0; i < N_PT_PER_FRAME; ++i) {
        payload.data[i] = i;
    }

    if (rte_eth_macaddr_get (port, &(ether_hdr.src_addr))) {
        std::cerr << "failed to get src addr" << std::endl;
        exit (-1);
    }

    ether_hdr.dst_addr = { send_cfg.dst_mac[0], send_cfg.dst_mac[1], send_cfg.dst_mac[2],
                           send_cfg.dst_mac[3], send_cfg.dst_mac[4], send_cfg.dst_mac[5] };
    ether_hdr.ether_type = rte_cpu_to_be_16 (0x0800);

    ipv4_hdr.ihl = 0x05;
    ipv4_hdr.version = 0x04; // IPV4
    ipv4_hdr.type_of_service = 0;
    ipv4_hdr.total_length =
    rte_cpu_to_be_16 (static_cast<uint16_t> (ip_pkt_len ())); // size of IPV4 header and everything that follows
    ipv4_hdr.packet_id = 0;
    ipv4_hdr.fragment_offset = 0x40; // don't fragment 0b0100_0000
    ipv4_hdr.time_to_live = IPDEFTTL; // default 64
    ipv4_hdr.next_proto_id = IPPROTO_UDP; // UDP packet follows
    ipv4_hdr.src_addr = rte_cpu_to_be_32 ((send_cfg.src_ip[0] << 24) + (send_cfg.src_ip[1] << 16) +
                                          (send_cfg.src_ip[2] << 8) + send_cfg.src_ip[3]);
    ipv4_hdr.dst_addr = rte_cpu_to_be_32 ((send_cfg.dst_ip[0] << 24) + (send_cfg.dst_ip[1] << 16) +
                                          (send_cfg.dst_ip[2] << 8) + send_cfg.dst_ip[3]);

    ipv4_hdr.hdr_checksum = 0;
    // ipv4_hdr.hdr_checksum = rte_ipv4_cksum (&ipv4_hdr);
    //  ipv4_hdr.hdr_checksum = 0xd7e3;

    udp_hdr.src_port = rte_cpu_to_be_16 (send_cfg.src_port);
    udp_hdr.dst_port = rte_cpu_to_be_16 (send_cfg.dst_port);
    udp_hdr.dgram_len = rte_cpu_to_be_16 (udp_pkt_len ());
    udp_hdr.dgram_cksum = 0;
    // udp_hdr.dgram_cksum = rte_ipv4_udptcp_cksum (&ipv4_hdr, &udp_hdr);


    /* Main work of application loop. 8< */
    // uint64_t cnt = 0;
    // std::time_t t0 = std::time (nullptr);
    auto old_ms = std::chrono::duration_cast<std::chrono::milliseconds> (
                  std::chrono::system_clock::now ().time_since_epoch ())
                  .count ();
    auto t0_ms = old_ms;
    uint64_t nbytes;
    for (int cnt = 0;; ++cnt) {
        /*
         * Receive packets on a port and forward them on the paired
         * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
         */


        /* Get burst of RX packets, from first port of pair. */
        struct rte_mbuf *bufs[BURST_SIZE];
        // const uint16_t nb_rx = rte_eth_rx_burst (port, 0, bufs, BURST_SIZE);

        if (rte_pktmbuf_alloc_bulk (mbuf_pool, bufs, BURST_SIZE) != 0) {
            std::cerr << "alloc err" << std::endl;
        }

        for (int i = 0; i < BURST_SIZE; ++i) {
            bufs[i]->pkt_len = ether_pkt_len ();
            bufs[i]->data_len = ether_pkt_len ();
            // uint64_t *pcnt = rte_pktmbuf_mtod (bufs[i], uint64_t *);

            pack_data (bufs[i], &ether_hdr, &ipv4_hdr, &udp_hdr, &payload);

            char *ptr = rte_pktmbuf_mtod (bufs[i], char *);
            auto udp_hdr1 = (rte_udp_hdr *)(ptr + sizeof (rte_ether_hdr) + sizeof (rte_ipv4_hdr));
            auto udp_offset = sizeof (rte_ether_hdr) + sizeof (rte_ipv4_hdr);
            // if (cnt%10000==0) std::cout<<udp_hdr1->dgram_cksum<<" "<<udp_hdr.dgram_cksum<<std::endl;
            udp_hdr1->dgram_cksum = 0;
            // udp_hdr1->dgram_cksum = rte_ipv4_udptcp_cksum_mbuf (bufs[i], &ipv4_hdr, udp_offset);
            payload.pkt_cnt += 1;
        }


        /* Send burst of TX packets, to second port of pair. */
        // const uint16_t nb_tx = rte_eth_tx_burst (port, 0, bufs, BURST_SIZE);
        uint16_t nb_tx = 0;
        do {
            nb_tx += rte_eth_tx_burst (port, 0, bufs + nb_tx, BURST_SIZE - nb_tx);
        } while (nb_tx != BURST_SIZE);

        nbytes += BURST_SIZE * ether_pkt_len ();

        /* Free any unsent packets. */
        assert (nb_tx == BURST_SIZE);


        auto new_ms = std::chrono::duration_cast<std::chrono::milliseconds> (
                      std::chrono::system_clock::now ().time_since_epoch ())
                      .count ();

        if (new_ms / 1000 != old_ms / 1000) {
            // double secs = std::difftime (time (nullptr), t0);
            double secs = (new_ms - t0_ms) / 1000;
            double Bps = nbytes / secs;
            std::cerr << std::setprecision (4) << "t elapsed= " << secs
                      << " sec, TX speed: " << Bps / 1e9 << " GBps = " << Bps * 8 / 1e9
                      << " Gbps = " << Bps / 1e6 / 2 << " MSps " << payload_len () << std::endl;
        }
        old_ms = new_ms;
        // break;
    }
    /* >8 End of loop. */
}
/* >8 End Basic forwarding application lcore. */

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int main (int argc, char *argv[])
{
    struct rte_mempool *mbuf_pool;
    unsigned nb_ports;
    uint16_t portid;

    /* Initializion the Environment Abstraction Layer (EAL). 8< */
    int ret = rte_eal_init (argc, argv);
    if (ret < 0) rte_exit (EXIT_FAILURE, "Error with EAL initialization\n");
    /* >8 End of initialization the Environment Abstraction Layer (EAL). */

    argc -= ret;
    argv += ret;

    std::cout << "argc:" << argc << std::endl;
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << std::endl;
    }

    if (argc < 1) {
        std::cerr << "Usage: " << argv[0] << " <cfg file>" << std::endl;
        exit (-1);
    }

    YAML::Node config = YAML::LoadFile (argv[1]);

    // std::cout<<"x="<<x[0]<<std::endl;
    // SendCfg send_cfg{ { 192, 168, 10, 10 }, 3000, { 192, 168, 10, 11 }, 3001, { 0xec, 0x0d, 0x9a, 0x43, 0x2e, 0x4d } };
    SendCfg send_cfg (config);

    /* Check that there is an even number of ports to send/receive on. */
    nb_ports = rte_eth_dev_count_avail ();
    if (nb_ports < 1) rte_exit (EXIT_FAILURE, "Error: number of ports must be >0 \n");

    /* Creates a new mempool in memory to hold the mbufs. */

    /* Allocates mempool to hold the mbufs. 8< */
    mbuf_pool = rte_pktmbuf_pool_create ("MBUF_POOL", NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
                                         MBUF_SIZE, rte_socket_id ());
    /* >8 End of allocating mempool to hold mbuf. */

    if (mbuf_pool == NULL) rte_exit (EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Initializing all ports. 8< */
    RTE_ETH_FOREACH_DEV (portid)
    if (port_init (portid, mbuf_pool) != 0)
        rte_exit (EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", portid);
    /* >8 End of initializing all ports. */

    if (rte_lcore_count () > 1) printf ("\nWARNING: Too many lcores enabled. Only 1 used.\n");

    /* Call lcore_main on the main core only. Called on single lcore. 8< */
    lcore_main (mbuf_pool, send_cfg);
    /* >8 End of called on single lcore. */

    /* clean up the EAL */
    rte_eal_cleanup ();

    return 0;
}
