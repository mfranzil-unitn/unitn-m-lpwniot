/*---------------------------------------------------------------------------*/
/*                                   LAB 7                                   */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "net/rime/collect.h"
#include "leds.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
#define MSG_PERIOD (30 * CLOCK_SECOND)  // send every 30 seconds
#define COLLECT_CHANNEL 0xAA
#define COLLECT_NUM_RTX 8
/*---------------------------------------------------------------------------*/
linkaddr_t sink = {{0x01, 0x00}}; // node 1 will be our sink
/*---------------------------------------------------------------------------*/
PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);
/*---------------------------------------------------------------------------*/
/* Application packet */
typedef struct {
  uint16_t seqn;
}
__attribute__((packed))
test_msg_t;
/*---------------------------------------------------------------------------*/
static struct collect_conn tc;
static void recv_cb(const linkaddr_t *originator, uint8_t seqno, uint8_t hops);
struct collect_callbacks cb = {.recv = recv_cb};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data) 
{
  static struct etimer periodic;
  static struct etimer rnd;
  static test_msg_t msg = {.seqn=0};

  PROCESS_BEGIN();

  if(linkaddr_cmp(&sink, &linkaddr_node_addr)) {
    printf("App: I am sink %02x:%02x\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
    /* Open Collect connection */
    collect_open(&tc, COLLECT_CHANNEL, COLLECT_ROUTER, &cb);
    collect_set_sink(&tc, 1);
  }
  else {
    printf("App: I am a normal node %02x:%02x\n",
      linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
    collect_open(&tc, COLLECT_CHANNEL, COLLECT_ROUTER, NULL);
    etimer_set(&periodic, MSG_PERIOD);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));
      /* Fixed interval */
      etimer_reset(&periodic);
      /* Random shift within the first half of the interval */
      etimer_set(&rnd, random_rand() % (MSG_PERIOD / 2));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&rnd));

      /* Send data collection packet to sink */
      packetbuf_clear();
      memcpy(packetbuf_dataptr(), &msg, sizeof(msg));
      packetbuf_set_datalen(sizeof(msg));
      printf("App: Send seqn %d\n", msg.seqn);
      collect_send(&tc, COLLECT_NUM_RTX);
      msg.seqn++;
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
recv_cb(const linkaddr_t *originator, uint8_t seqno, uint8_t hops)
{
  test_msg_t msg;
  if (packetbuf_datalen() != sizeof(msg)) {
    printf("App: wrong length: %d\n", packetbuf_datalen());
    return;
  }
  memcpy(&msg, packetbuf_dataptr(), sizeof(msg));
  printf("App: Recv from %02x:%02x seqn %d hops %d\n",
    originator->u8[0], originator->u8[1], msg.seqn, hops);
}
/*---------------------------------------------------------------------------*/
