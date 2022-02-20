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
#include "core/net/linkaddr.h"
#include "rng-support.h"
/*---------------------------------------------------------------------------*/
PROCESS(ranging_resp_process, "Ranging responder process");
AUTOSTART_PROCESSES(&ranging_resp_process);
/*---------------------------------------------------------------------------*/
/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
 * 1 uus = 512 / 499.2 µs and 1 µs = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME (65536)
#define RESP_DELAY (500) // wait time for ranging reply in ~us
#define UWB_RESP_DELAY (RESP_DELAY * UUS_TO_DWT_TIME)
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ranging_resp_process, ev, data)
{
  static sstwr_init_msg_t init_msg; // first ranging packet
  static sstwr_resp_msg_t resp_msg; // reply to ranging packet
  static uint8_t ret; // check transmission status

  PROCESS_BEGIN();

  printf("I am %02x%02x\n",
    linkaddr_node_addr.u8[0],
    linkaddr_node_addr.u8[1]
  );

  radio_init();

  while(1) {

    /* Start the receiver immediately */
    start_rx(NO_RX_TIMEOUT);
    printf("RX enabled\n");

    /* Wait for response */
    ret = wait_rx();

    /* If reception failed, restart */
    if (!ret) {
      radio_reset();
      printf("RX fail\n");
      continue;
    }

    /* Read the ranging init message */
    ret = read_rx_data(&init_msg, sizeof(init_msg));

    /* If the message could not be read, abort */
    if (!ret) {
      radio_reset();
      printf("RX wrong size\n");
      continue;
    }

    /* Get packet source and destination linkaddr */
    linkaddr_t init_src, init_dst;
    init_src.u8[0] = init_msg.hdr.src[1];
    init_src.u8[1] = init_msg.hdr.src[0];
    init_dst.u8[0] = init_msg.hdr.dst[1];
    init_dst.u8[1] = init_msg.hdr.dst[0];

    /* Check if the INIT destination is this node;
     * in that case, reply to complete the two-way ranging procedure */
    if(linkaddr_cmp(&init_dst, &linkaddr_node_addr)) {

      /* Get RX timestamp and compute TX time to schedule resp transmission */
      uint64_t rx_ts = get_rx_timestamp();
      uint64_t tx_ts = rx_ts + UWB_RESP_DELAY;

      /* Set addresses and ranging sequence number in resp message */
      fill_ieee_hdr(&resp_msg.hdr, &linkaddr_node_addr, &init_src, init_msg.hdr.seqn);

      /* Predict actual TX timestamp BEFORE sending */
      uint64_t predicted_tx_ts = predict_tx_timestamp(tx_ts);

      /* Write timestamps in the resp message */
      resp_msg_set_timestamp(&resp_msg.init_rx_ts[0], rx_ts);
      resp_msg_set_timestamp(&resp_msg.resp_tx_ts[0], predicted_tx_ts);

      /* Send packet and wait for TX confirmation */
      ret = start_tx(
        &resp_msg, sizeof(resp_msg),
        DWT_START_TX_DELAYED,
        0,
        tx_ts
      );

      /* Check outcome of transmission */
      if(ret) {
        printf("[%u] RESP %02x%02x at %llu (ts: %llu->%llu)\n",
          init_msg.hdr.seqn, resp_msg.hdr.dst[0], resp_msg.hdr.dst[1],
          get_tx_timestamp(), rx_ts, predicted_tx_ts);
      }
      else {
        printf("[%u] RESP fail %02x%02x\n",
          init_msg.hdr.seqn, resp_msg.hdr.dst[0], resp_msg.hdr.dst[1]);
      }
    }
    else {
      printf("RX wrong dest %02x%02x\n", init_msg.hdr.dst[0], init_msg.hdr.dst[1]);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
