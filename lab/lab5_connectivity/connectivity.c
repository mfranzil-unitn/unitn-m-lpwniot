
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include <stdbool.h>

/* Application Configuration */
#define BROADCAST_CHANNEL 0xAA
#define BEACON_INTERVAL (5 * CLOCK_SECOND)
#define RANDOM_INTERVAL (random_rand() % (5 * CLOCK_SECOND))

PROCESS(connect_process, "Connectivity Process");
AUTOSTART_PROCESSES(&connect_process);

static struct broadcast_conn bc_conn;
static void recv_bc(struct broadcast_conn *c, const linkaddr_t *from);
static void sent_bc(struct broadcast_conn *c, int status, int num_tx);
static const struct broadcast_callbacks bc_callbacks = {
  .recv     = recv_bc,
  .sent     = sent_bc,
};

PROCESS_THREAD(connect_process, ev, data)
{
  static struct etimer et_interval;
  static struct etimer et_random;
  PROCESS_BEGIN();
  printf("Node Link Layer Address: %X:%X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  broadcast_open(&bc_conn, BROADCAST_CHANNEL, &bc_callbacks);

  /* Wait for other nodes to start */
  etimer_set(&et_interval, 5 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&et_interval));

  /* Send broadcast beacons to assess connectivity */
  etimer_set(&et_interval, BEACON_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    if(etimer_expired(&et_interval)) {
      etimer_set(&et_random, RANDOM_INTERVAL);
      etimer_reset(&et_interval);
    }
    else if(etimer_expired(&et_random)) {
      packetbuf_clear();
      broadcast_send(&bc_conn); // empty message
    }
  }
  PROCESS_END();
}

static void
recv_bc(struct broadcast_conn *c, const linkaddr_t *from)
{
  radio_value_t rssi;

  NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);

  printf("RX %02x:%02x->%02x:%02x, RSSI = %ddBm\n",
    from->u8[0], from->u8[1],
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
    rssi);
}

static void
sent_bc(struct broadcast_conn *c, int status, int num_tx)
{
  printf("TX %02x:%02x\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
}

