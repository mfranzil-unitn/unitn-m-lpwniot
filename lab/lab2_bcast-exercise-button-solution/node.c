
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

#define LOG_VERBOSE 1

PROCESS(broadcast_process, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast_process);

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  size_t len = packetbuf_datalen();
  char msg[len + 1];                      // Allocate an array of the appropriate size
  memcpy(msg, packetbuf_dataptr(), len);  // Copy the received data to the array
  msg[len] = '\0';                        // Zero-terminate the string (just in case)

  printf("Recv from %02X:%02X Message: '%s'\n",
         from->u8[0], from->u8[1], msg);
}

static void
broadcast_sent(struct broadcast_conn *c, int status, int num_tx)
{
#if LOG_VERBOSE == 1
  printf("Broadcast message sent STATUS = %d, NUM_TX = %d\n",
        status, num_tx);
#endif
}

static const struct broadcast_callbacks broadcast_cb = {
  .recv = broadcast_recv,
  .sent = broadcast_sent
};
static struct broadcast_conn broadcast;

PROCESS_THREAD(broadcast_process, ev, data)
{
  static struct etimer et;
  static uint8_t send_other_msg = 0;
  static char msg[] = "Hello from Matteo!";
  static char other_msg[] = "Hello from Davide!";

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  /* Open the Rime broadcast primitive */
  broadcast_open(&broadcast, 129, &broadcast_cb);

  /* Print node link layer address */
  printf("Node Link Layer Address: %02X:%02X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  while(1) {

    /* Delay 5-8 seconds */
    etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3));

    /* Wait for the timer to expire, or the button to be pressed */
    while(1) {
      PROCESS_WAIT_EVENT();
      if(ev == PROCESS_EVENT_TIMER && etimer_expired(&et)) {
        break;
      }
      else if(ev == sensors_event && data == &button_sensor) {

        /* Toggle the transmission of the other msg;
         * note that this implementations still waits until the etimer expires
         * before transmitting. You can use "break;" here if you want to
         * exit from the while loop and transmit immediately. */
        send_other_msg = !send_other_msg;
      }
    }

    /* Load buffer with the selected message */
    packetbuf_clear();
    if(send_other_msg) {
      packetbuf_copyfrom(other_msg, strlen(other_msg));
    }
    else {
      packetbuf_copyfrom(msg, strlen(msg));
    }

    /* Send the message */
#if LOG_VERBOSE == 1
    printf("Sending broadcast message (source address: %02X:%02X)\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
#endif
    broadcast_send(&broadcast);
  }

  PROCESS_END();
}

