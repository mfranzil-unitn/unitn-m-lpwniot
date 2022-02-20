#include "dw1000.h"
#include "net/netstack.h"
#include "dev/watchdog.h"
#include "rng-support.h"
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
uint64_t
get_rx_timestamp(void)
{
  uint8_t ts_tab[5];
  uint64_t ts = 0;
  int i;
  dwt_readrxtimestamp(ts_tab);
  for(i = 4; i >= 0; i--) {
    ts <<= 8;
    ts |= ts_tab[i];
  }
  return ts;
}
/*---------------------------------------------------------------------------*/
uint64_t
get_tx_timestamp(void)
{
  uint8_t ts_tab[5];
  uint64_t ts = 0;
  int i;
  dwt_readtxtimestamp(ts_tab);
  for(i = 4; i >= 0; i--) {
    ts <<= 8;
    ts |= ts_tab[i];
  }
  return ts;
}
/*---------------------------------------------------------------------------*/
uint64_t
predict_tx_timestamp(uint64_t tx_ts) {

  /* Obtain the actual transmission time of the DW1000 */
  return (tx_ts & DWT_TX_BITMASK) + ANTENNA_DELAY;
}
/*---------------------------------------------------------------------------*/
void
resp_msg_set_timestamp(uint8_t *ts_field, uint64_t ts)
{
  /* Embed timestamp byte by byte */
  int i;
  for(i = 0; i < RNG_TS_LEN; i++) {
    ts_field[i] = (ts >> (i * 8)) & 0xFF;
  }
}
/*---------------------------------------------------------------------------*/
void
resp_msg_get_timestamp(uint8_t *ts_field, uint64_t *ts)
{
  /* Read timestamp byte by byte */
  int i;
  *ts = 0;
  for(i = 0; i < RNG_TS_LEN; i++) {
    *ts += ts_field[i] << (i * 8);
  }
}
/*---------------------------------------------------------------------------*/
void
fill_ieee_hdr(ieee154_hdr_t *hdr, linkaddr_t *src, linkaddr_t *dst, uint8_t seqn)
{
  /* Fill in the constant part of the TX buffer */
  hdr->fctrl[0] = 0x41;
  hdr->fctrl[1] = 0x88;
  hdr->seqn = seqn;
  hdr->pan_id[0] = IEEE802154_PANID & 0xff;
  hdr->pan_id[1] = IEEE802154_PANID >> 8;
  if(src != NULL) {
    hdr->src[0] = src->u8[1];
    hdr->src[1] = src->u8[0];
  }
  if(dst != NULL) {
    hdr->dst[0] = dst->u8[1];
    hdr->dst[1] = dst->u8[0];
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
start_tx(void *data, uint8_t len, uint8_t mode, uint16_t rx_to, uint64_t tx_time)
{
  /* Force transceiver off to avoid having the receiver enabled */
  dwt_forcetrxoff();

  /* Clean event flags */
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS | RX_WAIT_FLAGS);

  /* Set RX delay after transmission and RX timeout */
  if (mode & DWT_RESPONSE_EXPECTED) {
    dwt_setrxaftertxdelay(0);
    dwt_setrxtimeout(rx_to);
  }

  /* Schedule delayed transmission */
  if (mode & DWT_START_TX_DELAYED) {
    dwt_setdelayedtrxtime((uint32_t)((tx_time & DWT_TX_BITMASK) >> 8));
  }
  else {
    dwt_setdelayedtrxtime(0);
  }

  /* Write packet in TX buffer */
  if(dwt_writetxdata(len + CRC_LEN, (uint8_t*)data, 0) == DWT_SUCCESS) {

    /* Set ranging flag */
    dwt_writetxfctrl(len + CRC_LEN, 0, 1);

    /* errata TX-1: force rx timeout */
    dwt_write8bitoffsetreg(PMSC_ID, PMSC_CTRL0_OFFSET, PMSC_CTRL0_TXCLKS_125M);

    /* Transmit */
    if(dwt_starttx(mode) == DWT_SUCCESS) {
      while(!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
        watchdog_periodic();
      }

      /* Clean TX event */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
      return 1;
    }
  }

  /* Transmission failed (insufficient time to schedule) */
  PRINTF("TX failed. Status: %lx\n", dwt_read32bitreg(SYS_STATUS_ID));
  return 0;
}
/*---------------------------------------------------------------------------*/
void
radio_init() {

  /* Force transceiver off */
  dwt_forcetrxoff();

  /* Disable the driver callbacks */
  dwt_setcallbacks(0, 0, 0, 0);

  /* Disable all interrupts */
  dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO |
    DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT  | DWT_INT_ARFE, 0);

  /* Set antenna delays */
  dwt_setrxantennadelay(ANTENNA_DELAY);
  dwt_settxantennadelay(ANTENNA_DELAY);

  /* Disable frame filtering */
  dwt_enableframefilter(DWT_FF_NOTYPE_EN);
}
/*---------------------------------------------------------------------------*/
void
start_rx(uint16_t rx_to) {
  dwt_setrxtimeout(rx_to);
  dwt_rxenable(DWT_START_RX_IMMEDIATE);
}
/*---------------------------------------------------------------------------*/
uint8_t
wait_rx()
{
  uint32_t status_reg;

  /* Wait for any RX result flag to be set by the radio */
  while(!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & RX_WAIT_FLAGS)) {
    watchdog_periodic();
  }

  /* Check RX outcome: SYS_STATUS_RXFCG indicates correct decoding */
  if (status_reg & SYS_STATUS_RXFCG) {
    return 1;
  }

  /* Reception failed (error or timeout) */
  PRINTF("RX failed. Status: %lx\n", status_reg);
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
read_rx_data(void* pkt, uint32_t pkt_size) {

  /* Check length of received packet and read the message */
  uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
  if (frame_len == pkt_size + CRC_LEN) {
    dwt_readrxdata((uint8_t *)pkt, pkt_size, 0);
#if DEBUG
    uint32_t i;
    printf("RECV ");
    for (i = 0; i < pkt_size; i++) {
      printf("%02x ", ((uint8_t *)pkt)[i]);
    }
    printf("\n");
#endif
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
radio_reset()
{
  /* Clean TX error/timeout events */
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS | RX_WAIT_FLAGS);

  /* Clear RX error/timeout events */
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

  /* Reset receiver */
  dwt_rxreset();

  /* Switch off transceiver */
  dwt_forcetrxoff();
}
/*---------------------------------------------------------------------------*/
