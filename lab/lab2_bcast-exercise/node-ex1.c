
#include <stdio.h>

#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"

#define MAX_MESSAGE_SIZE 40


PROCESS(broadcast_process, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast_process);

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    size_t len = packetbuf_datalen();
    char buf[len + 1];
    memcpy(buf, packetbuf_dataptr(), len);

    printf("Recv from %02X:%02X Message: '%s'\n",
           from->u8[0], from->u8[1], buf);

    packetbuf_clear();
}

static void broadcast_sent(struct broadcast_conn *conn, int status, int num_tx) {
    printf("Report - status: %d, TX: %d\n", status, num_tx);
}

static const struct broadcast_callbacks broadcast_cb = {
    .recv = broadcast_recv,
    .sent = broadcast_sent};

static struct broadcast_conn broadcast;

PROCESS_THREAD(broadcast_process, ev, data) {
    static struct etimer et;

    PROCESS_BEGIN();

    SENSORS_ACTIVATE(button_sensor);

    //printf(sensors_event.data==&button_sensor)

    char buf[MAX_MESSAGE_SIZE + 1];

    broadcast_open(&broadcast, 123, &broadcast_cb);

    /* Print node link layer address */
    printf("Node Link Layer Address: %02X:%02X\n",
           linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

    while (1) {
        /* Delay 5-8 seconds */
        etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3));

        /* Wait for the timer to expire */
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        //printf("ev == sensors_event : %d; data == &button_sensor: %d; button_sensor.value(0): %d", ev == sensors_event, data == &button_sensor, button_sensor.value(0));

        int i;
        for (i = 0; i < MAX_MESSAGE_SIZE; i++) {
            if (button_sensor.value(0) > 0) {
                buf[i] = (char)((random_rand() % 26) + 'a');
            } else {
                buf[i] = (char)((random_rand() % 26) + 'A');
            }
        }

        buf[MAX_MESSAGE_SIZE] = '\0';

        packetbuf_clear();
        packetbuf_copyfrom(&buf, MAX_MESSAGE_SIZE);

        broadcast_send(&broadcast);
    }

    PROCESS_END();
}

