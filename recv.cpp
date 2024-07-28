/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include "config.h"
#include <cinttypes>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include "payload.h"

using namespace std;
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32


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

    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

    std::cout << "mtu:" << port_conf.rxmode.mtu << std::endl;
    port_conf.rxmode.mtu = MTU;

    /* Configure the Ethernet device. */
    retval = rte_eth_dev_configure (port, rx_rings, tx_rings, &port_conf);
    if (retval != 0) return retval;

    // retval = rte_eth_dev_set_mtu(port, 9200);
    if (retval != 0) return retval;

    uint16_t x;
    rte_eth_dev_get_mtu (port, &x);
    std::cout << "MTU =" << x << std::endl;

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
static __rte_noreturn void lcore_main (uint16_t port)
{

    /*
     * Check that the port is on the same NUMA node as the polling thread
     * for best performance.
     */

    if (rte_eth_dev_socket_id (port) >= 0 && rte_eth_dev_socket_id (port) != (int)rte_socket_id ())
        printf ("WARNING, port %u is on remote NUMA node to "
                "polling thread.\n\tPerformance will "
                "not be optimal.\n",
                port);

    printf ("\nCore %u forwarding packets. [Ctrl+C to quit]\n", rte_lcore_id ());

    /* Main work of application loop. 8< */
    uint64_t old_cnt = 0;
    std::time_t t0 = std::time (nullptr);

    rte_ether_hdr *ether_hdr;
    rte_ipv4_hdr *ipv4_hdr;
    rte_udp_hdr *udp_hdr;
    Payload *payload;

    uint64_t nbytes = 0;
    uint64_t ndropped = 0;
    uint64_t npkts = 0;
    for (int i = 0;;) {
        /*
         * Receive packets on a port and forward them on the paired
         * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
         */

        /* Get burst of RX packets, from first port of pair. */
        struct rte_mbuf *bufs[BURST_SIZE];
        const uint16_t nb_rx = rte_eth_rx_burst (port, 0, bufs, BURST_SIZE);

        if (unlikely (nb_rx == 0)) continue;

        ++i;

        /* Send burst of TX packets, to second port of pair. */

        /* Free any unsent packets. */
        // std::cout<<bufs[0]->pkt_len<<std::endl;
        for (int buf = 0; buf < nb_rx; buf++) {

            if (bufs[buf]->pkt_len == pkt_len ()) {
                unpack_data (bufs[buf], &ether_hdr, &ipv4_hdr, &udp_hdr, &payload);
                auto cnt = payload->pkt_cnt;
                if (cnt == 0) {
                    t0 = std::time (nullptr);
                    nbytes = 0;
                    npkts = 0;
                    ndropped = 0;
                }


                // std::cout << *pcnt << std::endl;
                if (cnt > 0 && npkts > 0 && old_cnt + 1 != cnt) {
                    int64_t ndropped1 = cnt - old_cnt - 1;
                    std::cerr << "dropped " << ndropped1 << " packets " << cnt << " " << old_cnt
                              << " " << npkts << std::endl;
                    ndropped += ndropped1;
                }


                if (cnt % 117187 == 0) {
                    // std::cout << "." << std::endl;
                    // double secs=double(clock()-t0)/double(CLOCKS_PER_SEC);
                    double secs = std::difftime (time (nullptr), t0);
                    double Bps = nbytes / secs;
                    std::cout << std::setprecision (4) << "t elapsed= " << secs
                              << " sec, RX speed: " << Bps / 1e9 << " GBps = " << Bps * 8 / 1e9
                              << " Gbps = " << Bps / 1e6 / 2 << " MSps, Dropped packet:" << ndropped
                              << " dropping ratio < " << (ndropped + 1.0) / npkts << std::endl;
                }
                old_cnt = cnt;
                npkts += 1;
                nbytes += pkt_len ();
            }

            rte_pktmbuf_free (bufs[buf]);
        }
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
    std::cout << "args" << std::endl;
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << std::endl;
    }

    /* Check that there is an even number of ports to send/receive on. */
    nb_ports = 1;

    /* Allocates mempool to hold the mbufs. 8< */
    mbuf_pool =
    rte_pktmbuf_pool_create ("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, MBUF_SIZE, rte_socket_id ());
    /* >8 End of allocating mempool to hold mbuf. */

    if (mbuf_pool == NULL) rte_exit (EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Initializing all ports. 8< */
    portid = 0;
    if (port_init (portid, mbuf_pool) != 0)
        rte_exit (EXIT_FAILURE, "Cannot init port %d \n", portid);
    /* >8 End of initializing all ports. */

    if (rte_lcore_count () > 1) printf ("\nWARNING: Too many lcores enabled. Only 1 used.\n");

    /* Call lcore_main on the main core only. Called on single lcore. 8< */
    lcore_main (portid);
    /* >8 End of called on single lcore. */

    /* clean up the EAL */
    rte_eal_cleanup ();

    return 0;
}
