#include "my_collect.h"
#include "contiki.h"
#include "core/net/linkaddr.h"
#include "leds.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include <stdbool.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#define BEACON_INTERVAL (CLOCK_SECOND * 60) /* Try to change this period to analyse \
                                             * how it affects the radio-on time     \
                                             * (energy consumption) of ContikiMac */
#define BEACON_FORWARD_DELAY (random_rand() % CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
#define RSSI_THRESHOLD -95
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
void beacon_timer_cb(void *ptr);
/*---------------------------------------------------------------------------*/
/* Rime Callback structures */
struct broadcast_callbacks bc_cb = {
    .recv = bc_recv,
    .sent = NULL};
struct unicast_callbacks uc_cb = {
    .recv = uc_recv,
    .sent = NULL};
/*---------------------------------------------------------------------------*/
void my_collect_open(struct my_collect_conn *conn, uint16_t channels,
                     bool is_sink, const struct my_collect_callbacks *callbacks) {
    /* TO DO 1.1 -- Initialise the connection structure: 
   * 1. Set the parent address to linkaddr_null;
   * 2. Set the metric field to its max value (65535) to indicate that
   *    the node is not connected yet;
   * 3. Set beacon_seqn to 0 as no beacon has been received yet;
   * 4. Check if the node is the sink;
   * 5. Set the callbacks field.
   */
    linkaddr_copy(&conn->parent, &linkaddr_null);
    conn->metric = 65535;
    conn->beacon_seqn = 0;
    conn->is_sink = is_sink;
    conn->callbacks = callbacks;

    /* Open the underlying Rime primitives */
    broadcast_open(&conn->bc, channels, &bc_cb);
    unicast_open(&conn->uc, channels + 1, &uc_cb);

    /* TO DO 1.2 -- SINK ONLY:
   * 1. Make the sink send beacons periodically (tip: use the ctimer in my_collect_conn).
   *    The FIRST time make the sink TX a beacon after 1 second, after that the sink
   *    should send beacons with a period equal to BEACON_INTERVAL.
   * 2. Does the sink need to change/update any field in the connection structure?
   */
    if (conn->is_sink) {
        conn->metric = 0; /* The sink hop count is (by definition) *always* equal to 0.
                       * Remember to update this field *before sending the first*
                       * beacon message in broadcast! */
        /* Schedule the first beacon message flood */
        ctimer_set(&conn->beacon_timer, CLOCK_SECOND, beacon_timer_cb, conn);
    }
}
/*---------------------------------------------------------------------------*/
/*                              Beacon Handling                              */
/*---------------------------------------------------------------------------*/
/* Beacon message structure */
struct beacon_msg {
    uint16_t seqn;
    uint16_t metric;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn and metric */
void send_beacon(struct my_collect_conn *conn) {
    /* Prepare the beacon message */
    struct beacon_msg beacon = {
        .seqn = conn->beacon_seqn, .metric = conn->metric};

    /* Send the beacon message in broadcast */
    packetbuf_clear();
    packetbuf_copyfrom(&beacon, sizeof(beacon));
    printf("my_collect: sending beacon: seqn %d metric %d\n",
           conn->beacon_seqn, conn->metric);
    broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Beacon timer callback */
void beacon_timer_cb(void *ptr) {
    /* TO DO 2 -- Implement the beacon callback:
   * 1. Send beacon (use send_beacon());
   * 2. Should the sink do anything else?
   */

    /* Common nodes and sink logic */
    struct my_collect_conn *conn = (struct my_collect_conn *)ptr; /* [Side note] Even an implicit cast
                                                                * struct my_collect_conn* conn = ptr;
                                                                * works correctly */
    send_beacon(conn);

    /* Sink-only logic */
    if (conn->is_sink) {
        /* Rebuild the three from scratch after the beacon interval */
        conn->beacon_seqn++; /* Before beginning a new beacon flood, the sink 
                           * should ALWAYS increase the beacon sequence number! */
        /* Schedule the next beacon message flood */
        ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, beacon_timer_cb, conn);
    }
}
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender) {
    struct beacon_msg beacon;
    struct my_collect_conn *conn;
    int16_t rssi;

    /* Get the pointer to the overall structure my_collect_conn from its field bc */
    conn = (struct my_collect_conn *)(((uint8_t *)bc_conn) -
                                      offsetof(struct my_collect_conn, bc));

    /* Check if the received broadcast packet looks legitimate */
    if (packetbuf_datalen() != sizeof(struct beacon_msg)) {
        printf("my_collect: broadcast of wrong size\n");
        return;
    }
    memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));

    /* TO DO 3.0:
   * Read the RSSI of the *last* reception.
   */
    rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    printf("my_collect: recv beacon from %02x:%02x seqn %u metric %u rssi %d\n",
           sender->u8[0], sender->u8[1],
           beacon.seqn, beacon.metric, rssi);

    /* TO DO 3.1:
   * 1. Analyze the received beacon message based on RSSI, seqn, and metric.
   * 2. Update (if needed) the node's current routing info (parent, metric, beacon_seqn).
   *    TIP: when you update the node's current routing info add a debug print, e.g.,
   *         printf("my_collect: new parent %02x:%02x, my metric %d\n", 
   *             sender->u8[0], sender->u8[1], conn->metric);
   */
    if (rssi < RSSI_THRESHOLD || beacon.seqn < conn->beacon_seqn)
        return;                             // The beacon is either too weak or too old, ignore it
    if (beacon.seqn == conn->beacon_seqn) { // The beacon is not new, check the metric
        if (beacon.metric + 1 >= conn->metric)
            return; // Worse or equal than what we have, ignore it
    }
    /* Otherwise, memorize the new parent, the metric, and the seqn */
    linkaddr_copy(&conn->parent, sender);
    conn->metric = beacon.metric + 1;
    conn->beacon_seqn = beacon.seqn;
    printf("my_collect: new parent %02x:%02x, my metric %d\n",
           sender->u8[0], sender->u8[1], conn->metric);

    /* TO DO 4:
   * If the metric or the seqn has been updated, retransmit *after a small, random 
   * delay* (BEACON_FORWARD_DELAY) the beacon message in broadcast, to update the 
   * node's neighbors about the routing changes
   */

    /* Schedule beacon propagation */
    ctimer_set(&conn->beacon_timer, BEACON_FORWARD_DELAY, beacon_timer_cb, conn);
}

/*---------------------------------------------------------------------------*/
/*                               Data Handling                               */
/*---------------------------------------------------------------------------*/
/* Header structure for data packets */
struct collect_header {
    linkaddr_t source;
    uint8_t hops;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Data Collection: 
 * start sending data towards the sink (first step: source --> source's parent) */
int my_collect_send(struct my_collect_conn *conn) {
    struct collect_header hdr = {.source = linkaddr_node_addr, .hops = 0};

    /* TO DO 5:
   * 1. Check if the node is connected (has a parent), if not return -1;
   * 2. Allocate space for the data collection header;
   * 3. Insert the header in the packet buffer;
   * 4. Send the packet to the parent using unicast.
   */

    /* 5.1 */
    if (linkaddr_cmp(&conn->parent, &linkaddr_null)) // The node is still disconnected
        return -1;                                   // Inform the app that my_connect is currently unable to forward/deliver the packet
    /* 5.2 */
    if (packetbuf_hdralloc(sizeof(struct collect_header))) {
        /* 5.3 */
        memcpy(packetbuf_hdrptr(), &hdr, sizeof(hdr));
        /* 5.4 */
        return unicast_send(&conn->uc, &conn->parent);
    } else         // Packetbuf_hdralloc() was unable to allocate sufficient header space (payload too big)
        return -2; // Inform the app that an issue with packetbuf_hdralloc() has occurred
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from) {
    struct my_collect_conn *conn;
    struct collect_header hdr;

    /* Get the pointer to the overall structure my_collect_conn from its field uc */
    conn = (struct my_collect_conn *)(((uint8_t *)uc_conn) -
                                      offsetof(struct my_collect_conn, uc));

    /* Check if the received unicast message looks legitimate */
    if (packetbuf_datalen() < sizeof(struct collect_header)) {
        printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
        return;
    }

    /* TO DO 6:
   * 1. Extract the header
   * 2. On the sink, remove the header and call the application callback. 
   *    Should we update any field of hdr?
   * 3. On a forwarder, update the header and forward the packet to the parent using unicast
   */

    /* 6.1 */
    memcpy(&hdr, packetbuf_dataptr(), sizeof(hdr));
    hdr.hops = hdr.hops + 1; // Upon receiving a data packet, the hop count header field needs to be always updated

    /* Potential debug print, to help you discover misbehaviours in your protocol: 
   * printf("my_collect: data packet rcvd -- source: %02x:%02x, hops: %u\n",
   *   hdr.source.u8[0], hdr.source.u8[1], hdr.hops);
   */

    /* 6.2 */
    if (conn->is_sink) { /* The destination has been reached, no more forwarding is needed! 
                        * Let's inform the application about the received packet. */
        /* Remove the header, to make packetbuf_dataptr() point to the beginning of the application payload */
        if (packetbuf_hdrreduce(sizeof(struct collect_header)))
            conn->callbacks->recv(&hdr.source, hdr.hops); // Call the sink recv callback function
        else
            printf("my_collect: ERROR, the header could not be reduced!");
    }
    /* 6.3 */
    else {                                                 /* Non-sink node acting as a forwarder. Send the received packet to the node's parent in unicast */
        if (linkaddr_cmp(&conn->parent, &linkaddr_null)) { /* Just to be sure, and to detect potential bugs. 
                                                         * If the node is disconnected, my-collect is unable 
                                                         * to forward the data packet upwards */
            printf("my_collect: ERROR, unable to forward data packet -- "
                   "source: %02x:%02x, hops: %u\n",
                   hdr.source.u8[0], hdr.source.u8[1], hdr.hops);
            return;
        }
        memcpy(packetbuf_dataptr(), &hdr, sizeof(hdr)); // Update the my-connect header in the packet buffer
        unicast_send(&conn->uc, &conn->parent);         // Send the updated message to the node's parent in unicast
    }
}
/*---------------------------------------------------------------------------*/