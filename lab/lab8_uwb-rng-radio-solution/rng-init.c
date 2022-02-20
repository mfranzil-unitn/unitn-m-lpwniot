/*
 * Copyright (c) 2020, University of Trento.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "dw1000.h"
#include "dw1000-util.h"
#include "dw1000-config.h"
#include "core/net/linkaddr.h"
#include "rng-support.h"
/*---------------------------------------------------------------------------*/
PROCESS(ranging_process, "Ranging process");
AUTOSTART_PROCESSES(&ranging_process);
/*---------------------------------------------------------------------------*/
#define RANGING_INTERVAL (CLOCK_SECOND / 2)
#define RANGING_TIMEOUT (5000) // max wait time for ranging reply, in ~us
#define CFO_CORRECTION 0
/*---------------------------------------------------------------------------*/
linkaddr_t init = {{0x11, 0x0c}}; // node 3
#define NUM_DEST 4
linkaddr_t resp_list[NUM_DEST] = {
  {{0x19, 0x15}}, // 1915 = node 2
  {{0x18, 0x33}}, // 1833 = node 7
  {{0x10, 0x89}}, // 1089 = node 8
  {{0x15, 0x95}}  // 1595 = node 36
};
linkaddr_t resp;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ranging_process, ev, data)
{
  static struct etimer et; // event timer
  static sstwr_init_msg_t init_msg; // first ranging packet
  static sstwr_resp_msg_t resp_msg; // reply to ranging packet
  static uint8_t seqn = 0; // sequence number
  static uint8_t ret; // check transmission status

  PROCESS_BEGIN();

  printf("I am %02x%02x\n",
    linkaddr_node_addr.u8[0],
    linkaddr_node_addr.u8[1]
  );

  radio_init();

  /* Keep ranging */
  while(1) {
    seqn ++;

    /* Choose destination (round robin) */
    linkaddr_copy(&resp, &resp_list[seqn % NUM_DEST]);

    /* Wait with the event timer */
    etimer_set(&et, RANGING_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&et));
    printf("[%u] ranging with %02x%02x ...\n", seqn, resp.u8[0], resp.u8[1]);

    /* Prepare packet header, setting the destination and sequence number */
    fill_ieee_hdr(&init_msg.hdr, &linkaddr_node_addr, &resp, seqn);

    /* Send packet */
    ret = start_tx(
      &init_msg, sizeof(init_msg), // content
      DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED, // mode
      RANGING_TIMEOUT, // maximum RX time after TX, if DWT_RESPONSE_EXPECTED
      0 // TX time not set because of DWT_START_TX_IMMEDIATE
    );

    /* If transmission failed, abort ranging round and restart procedure */
    if (!ret) {
      radio_reset();
      printf("[%u] fail (TX err)\n", seqn);
      continue;
    }

    /* Wait for response */
    ret = wait_rx();

    /* If reception failed, abort ranging round and restart procedure */
    if (!ret) {
      radio_reset();
      printf("[%u] fail (RX err)\n", seqn);
      continue;
    }

    /* Read the ranging reply */
    ret = read_rx_data(&resp_msg, sizeof(resp_msg));

    /* If the message could not be read, abort */
    if (!ret) {
      radio_reset();
      printf("[%u] fail (bad size)\n", seqn);
      continue;
    }

    /* Get linkaddr information in the packet of responder */
    linkaddr_t resp_src, resp_dst;
    resp_src.u8[0] = resp_msg.hdr.src[1];
    resp_src.u8[1] = resp_msg.hdr.src[0];
    resp_dst.u8[0] = resp_msg.hdr.dst[1];
    resp_dst.u8[1] = resp_msg.hdr.dst[0];

    /* Check that the addresses match */
    if(linkaddr_cmp(&resp_src, &resp) && linkaddr_cmp(&resp_dst, &linkaddr_node_addr)) {

      /* Get init TX and resp RX timestamps */
      uint64_t init_tx_ts, resp_rx_ts;
      init_tx_ts = get_tx_timestamp();
      resp_rx_ts = get_rx_timestamp();

      /* Get init RX and resp TX timestamps, embedded in response message */
      uint64_t init_rx_ts, resp_tx_ts;
      resp_msg_get_timestamp(&resp_msg.init_rx_ts[0], &init_rx_ts);
      resp_msg_get_timestamp(&resp_msg.resp_tx_ts[0], &resp_tx_ts);

#if CFO_CORRECTION
      /* Get clock frequency offset (CFO) */
      double cfo_ppm = dw1000_get_ppm_offset(dw1000_get_current_cfg());
      double cfo_ratio = cfo_ppm / 1.0e6;
#else
      double cfo_ratio = 0;
#endif

      /* Compute time of flight
       * Note: timestamps are expressed in DWT_TIME_UNITS (~15.65e-12 s)
       * INIT   init_tx_ts <---- t_one -----> resp_rx_ts
       *             \                            /
       *              \                          /
       * RESP     init_rx_ts <-- t_two ---> resp_tx_ts
       */
      int64_t t_one, t_two;
      double tof;
      t_one = resp_rx_ts - init_tx_ts;
      t_two = resp_tx_ts - init_rx_ts;
      tof = (t_one - t_two * (1 - cfo_ratio)) / 2.0 * DWT_TIME_UNITS;

      /* Based on time of flight, compute distance */
      double dist;
      uint64_t dist_mm;
      dist = tof * SPEED_OF_LIGHT;
      dist_mm = (uint64_t)(dist * 1000); // in millimeters

      printf("RANGING OK [%02x:%02x->%02x:%02x] %llu mm\n",
        linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
        resp.u8[0], resp.u8[1],
        dist_mm);
    }
    else {
      printf("[%u] fail [%02x%02x->%02x%02x]\n",
        seqn, resp_src.u8[0], resp_src.u8[1], resp_dst.u8[0], resp_dst.u8[1]);
      continue;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
