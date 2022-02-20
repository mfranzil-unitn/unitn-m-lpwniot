
#include <stdio.h>
#include <stdlib.h>

#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"

static struct ctimer timer;  // timer object

unsigned short current = -1;

static void timer_cb(void *);

PROCESS(broadcast_process, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast_process);

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    size_t len = packetbuf_datalen();
    char buf[len + 1];
    memcpy(buf, packetbuf_dataptr(), len);

    current = atoi(buf);
    current++;

    printf("Recv from %02X:%02X Message: '%s'\n",
           from->u8[0], from->u8[1], buf);

    ctimer_set(&timer, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3), timer_cb, NULL);

    packetbuf_clear();
}

static void broadcast_sent(struct broadcast_conn *conn, int status, int num_tx) {
}

static const struct broadcast_callbacks broadcast_cb = {
    .recv = broadcast_recv,
    .sent = broadcast_sent};

static struct broadcast_conn broadcast;

static void timer_cb(void *ptr) {
    char buf[sizeof(unsigned short)];
    snprintf(buf, sizeof(unsigned short), "%d", current);
    packetbuf_clear();
    packetbuf_copyfrom(&buf, sizeof(unsigned short));

    broadcast_send(&broadcast);
}

PROCESS_THREAD(broadcast_process, ev, data) {
    PROCESS_BEGIN();

    broadcast_open(&broadcast, 123, &broadcast_cb);

    printf("Node Link Layer Address: %02X:%02X\n",
           linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
           
    if (linkaddr_node_addr.u8[0] == 01) {
        char buf[sizeof(unsigned short)];
        int rnd = random_rand();
        snprintf(buf, sizeof(unsigned short), "%d", rnd);

        packetbuf_copyfrom(&buf, sizeof(unsigned short));

        broadcast_send(&broadcast);
    }

    // set the callback timer
    //ctimer_set(&timer, PERIOD*9/10, timer_cb, NULL);

    PROCESS_END();
}
