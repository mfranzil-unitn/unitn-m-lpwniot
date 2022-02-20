#include "my_collect.h"
#include "contiki.h"
#include "core/net/linkaddr.h"
#include "leds.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include <stdbool.h>
#include <stdio.h>

#define BEACON_INTERVAL (CLOCK_SECOND * 60)
#define BEACON_FORWARD_DELAY (random_rand() % CLOCK_SECOND)

#define RSSI_THRESHOLD -95

/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
void beacon_timer_cb(void *ptr);

/* Rime Callback structures */
struct broadcast_callbacks bc_cb = {
    .recv = bc_recv,
    .sent = NULL};
struct unicast_callbacks uc_cb = {
    .recv = uc_recv,
    .sent = NULL};

void my_collect_open(struct my_collect_conn *conn, uint16_t channels,
                     bool is_sink, const struct my_collect_callbacks *callbacks) {
    /* TO DO 1.1: Initialize the connection structure.
    * 1. Set the parent address to linkaddr_null;
    * 2. Set the metric field to its max value (65535) to indicate that
    *    the node is not connected yet;
    * 3. Set beacon_seqn to 0 as no beacon has been received yet;
    * 4. Check if the node is the sink;
    * 5. Set the callbacks field.
    */
    conn->parent = linkaddr_null;
    conn->metric = 65535;
    conn->beacon_seqn = 0;

    if (is_sink) {
        conn->is_sink = true;
    } else {
        conn->is_sink = false;
    }

    conn->callbacks = callbacks;

    /* Open the underlying Rime primitives */
    broadcast_open(&conn->bc, channels, &bc_cb);
    unicast_open(&conn->uc, channels + 1, &uc_cb);

    /* TO DO 1.2: SINK ONLY
   * 1. Make the sink send beacons periodically (tip: use the ctimer in my_collect_conn).
   *    The FIRST time make the sink TX a beacon after 1 second, after that the sink
   *    should send beacons with a period equal to BEACON_INTERVAL.
   * 2. Does the sink need to change/update any field in the connection structure?
   */

    if (is_sink) {
        conn->metric = 0;
        ctimer_set(&conn->beacon_timer, CLOCK_SECOND, beacon_timer_cb, conn);
    }
}

/*                              Beacon Handling                              */

/* Beacon message structure */
struct beacon_msg {
    uint16_t seqn;
    uint16_t metric;
} __attribute__((packed));

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

/* Beacon timer callback */
void beacon_timer_cb(void *ptr) {
    /* TO DO 2: Implement the beacon callback.
   * 1. Send beacon (use send_beacon());
   * 2. Should the sink do anything else?
   */

    struct my_collect_conn *conn = (struct my_collect_conn *)ptr;

    send_beacon(conn);

    if (conn->is_sink) {
        conn->beacon_seqn++;
        ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, beacon_timer_cb, conn);
    }
}

/* Beacon receive callback */
void bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender) {
    struct beacon_msg beacon;
    int16_t rssi;
    /* Get the pointer to the overall structure my_collect_conn from its field bc */
    struct my_collect_conn *conn = (struct my_collect_conn *)(((uint8_t *)bc_conn) - offsetof(struct my_collect_conn, bc));

    /* Check if the received broadcast packet looks legitimate */
    if (packetbuf_datalen() != sizeof(struct beacon_msg)) {
        printf("my_collect: broadcast of wrong size\n");
        return;
    }
    memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));

    /* TO DO 3.0:
   * Read the RSSI of the *last* reception
   */
    NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, rssi);

    printf("my_collect: recv beacon from %02x:%02x seqn %u metric %u rssi %d\n",
           sender->u8[0], sender->u8[1],
           beacon.seqn, beacon.metric, rssi);

    /* TO DO 3:
    * 1. Analyze the received beacon based on RSSI, seqn, and metric.
    * 2. Update (if the seqn is newer and the metric is better) the node's
    *    current routing info (parent, metric, beacon_seqn).
    *    TIP: when you update the node's current routing info add a debug print, e.g.,
    *         printf("my_collect: new parent %02x:%02x, my metric %d\n", 
    *             sender->u8[0], sender->u8[1], conn->metric);
    */

    if (rssi > RSSI_THRESHOLD) {
        if (beacon.seqn > conn->beacon_seqn && beacon.metric < conn->metric) {
            conn->parent = *sender;
            conn->metric = beacon.metric;
            conn->beacon_seqn = beacon.seqn;
            printf("my_collect: new parent %02x:%02x, my metric %d\n",
                   sender->u8[0], sender->u8[1], conn->metric);
        }
    }

    /* TO DO 4:
    * If the metric or the seqn has been updated, retransmit *after a small, random 
    * delay* (BEACON_FORWARD_DELAY) the beacon to update the node neighbors about the 
    * changes
    */

    if (beacon.seqn == conn->beacon_seqn || beacon.metric == conn->metric) {
        ctimer_set(&conn->beacon_timer, BEACON_FORWARD_DELAY, beacon_timer_cb, conn);
    }
}

/*                               Data Handling                               */

/* Header structure for data packets */
struct collect_header {
    linkaddr_t source;
    uint8_t hops;
} __attribute__((packed));

/* Data Collection: send function */
int my_collect_send(struct my_collect_conn *conn) {
    struct collect_header hdr = {.source = linkaddr_node_addr, .hops = 0};
    int ret;

    /* TO DO 5:
   * 1. Check if the node is connected (has a parent), if not return 0;
   * 2. Allocate space for the data collection header; 
   * 3. Insert the header in the packet buffer;
   * 4. Send the packet to the parent using unicast.
   */
}

/* Data receive callback */
void uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from) {
    /* Get the pointer to the overall structure my_collect_conn from its field uc */
    struct my_collect_conn *conn = (struct my_collect_conn *)(((uint8_t *)uc_conn) -
                                                              offsetof(struct my_collect_conn, uc));

    struct collect_header hdr;

    /* Check if the received unicast message looks legitimate */
    if (packetbuf_datalen() < sizeof(struct collect_header)) {
        printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
        return;
    }

    /* TO DO 6:
   * 1. Extract the header
   * 2. On the sink, remove the header and call the application callback
   * 3. On a router, update the header and forward the packet to the parent using unicast
   */
}
