
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "sys/node-id.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

/* Application Configuration */
#define UNICAST_CHANNEL 146
#define APP_TIMER_DELAY (CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2))

typedef struct ping_pong_msg {
  uint16_t number;
  int16_t noise_floor;
}
__attribute__((packed))
ping_pong_msg_t;

static linkaddr_t receiver = {{0xAA, 0xBB}};

static uint16_t ping_pong_number = 0;
static struct ctimer ct;

/* Declaration of static functions */
static void print_rf_conf(void);
static void set_ping_pong_msg(ping_pong_msg_t *m);
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from);
static void sent_unicast(struct unicast_conn *c, int status, int num_tx);

static const struct unicast_callbacks uc_callbacks = {
  .recv     = recv_unicast,
  .sent     = sent_unicast
};
static struct unicast_conn uc;

static void
print_rf_conf(void)
{
  radio_value_t rfval = 0;

  printf("RF Configuration:\n");
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &rfval);
  printf("\tRF Channel: %d\n", rfval);
  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &rfval);
  printf("\tTX Power: %ddBm\n", rfval);
  NETSTACK_RADIO.get_value(RADIO_PARAM_CCA_THRESHOLD, &rfval);
  printf("\tCCA Threshold: %d\n", rfval);
}

static void
set_ping_pong_msg(ping_pong_msg_t *m)
{
  radio_value_t rfval = 0;

  /* Set the number to set */
  m->number = ping_pong_number;

  /* Get noise floor and set it on the message */
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rfval);
  m->noise_floor = rfval;
}

static void
ct_cb(void *ptr)
{
  /* Build the message */
  ping_pong_msg_t msg;
  set_ping_pong_msg(&msg);

  /* Clear the buffer */
  packetbuf_clear();

  /* Copy the message to the buffer */
  packetbuf_copyfrom(&msg, sizeof(ping_pong_msg_t));

  /* Send packet to receiver using unicast */
  unicast_send(&uc, &receiver);
}

static void
recv_unicast(struct unicast_conn *c, const linkaddr_t *from)
{
  /* Local variables */
  ping_pong_msg_t msg;
  radio_value_t rssi;

  /* Set the callback timer */
  ctimer_set(&ct, APP_TIMER_DELAY, ct_cb, NULL);

  /* Get received message */
  memcpy(&msg, packetbuf_dataptr(), sizeof(ping_pong_msg_t));

  /* Get received message, using packetbuf_copyto */
  // if(packetbuf_datalen() != sizeof(ping_pong_msg_t)) {
  //   printf("Node: recv unexpected Length = %u\n", packetbuf_datalen());
  //   return;
  // }
  // packetbuf_copyto(&msg);

  /* Get RSSI */
  NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);

  /* Print message and RSSI and LQI */
  printf("Node: recv from %02X:%02X Length = %u, RSSI = %ddBm, "
    "Noise Floor = %ddBm, Ping Pong Number = %u\n",
    from->u8[0], from->u8[1], packetbuf_datalen(), 
    rssi, msg.noise_floor, msg.number);

  /* Increase the Ping-pong number */
  ping_pong_number = msg.number + 1;
}

static void
sent_unicast(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("Node: unicast message sent to %02X:%02X, "
         "Status = %d, "
         "Number of Transmissions = %d, "
         "Ping Pong Number = %u\n",
    dest->u8[0], dest->u8[1], status, num_tx, ping_pong_number);  
}

PROCESS(unicast_process, "Unicast Process");
AUTOSTART_PROCESSES(&unicast_process);

PROCESS_THREAD(unicast_process, ev, data)
{
  PROCESS_BEGIN();

#if CONTIKI_TARGET_SKY
  /* Cooja Setup for Lab 3:
   * Set the right destination address for each
   * node in the network */
  if(node_id == 1) {
    receiver.u8[0] = 0x02;
    receiver.u8[1] = 0x00;
  } else if(node_id == 2) {
    receiver.u8[0] = 0x01;
    receiver.u8[1] = 0x00;
  }
#endif

  /* Print RF configuration */
  print_rf_conf();

  /* Print node link layer address */
  printf("Node Address: %02X:%02X\n",
    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);

  /* Print receiver link layer address */
  printf("Receiver address: %02X:%02X\n",
    receiver.u8[0], receiver.u8[1]);

  /* Print size of struct ping_pong_msg_t */
  printf("Size of struct: %u\n", sizeof(ping_pong_msg_t));

  /* Open the Rime unicast primitive */
  unicast_open(&uc, UNICAST_CHANNEL, &uc_callbacks);

  /* Set the callback timer */
  ctimer_set(&ct, APP_TIMER_DELAY, ct_cb, NULL);

  while(1) {
    /* Do nothing */
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

