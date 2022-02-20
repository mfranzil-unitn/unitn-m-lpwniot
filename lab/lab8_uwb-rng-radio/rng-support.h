#ifndef RNG_SUPPORT_H
#define RNG_SUPPORT_H
/*---------------------------------------------------------------------------*/
#include "deca_regs.h"
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
#define RX_WAIT_FLAGS (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)
#define NO_RX_TIMEOUT 0
#define RNG_TS_LEN 8 // ranging timestamps
#define DWT_TX_BITMASK (0xFFFFFFFE00UL)
#define ANTENNA_DELAY 16455
#define SPEED_OF_LIGHT 299702547 // in air, m/s
#define BUF_LEN 30
#define CRC_LEN 2
/*---------------------------------------------------------------------------*/

/* Message header, common for init and resp messages */
typedef struct {
  uint8_t fctrl[2];
  uint8_t seqn;
  uint8_t pan_id[2];
  uint8_t dst[2];
  uint8_t src[2];
} __attribute__ ((__packed__)) ieee154_hdr_t;

/* Ranging initiator message */
typedef struct {
  ieee154_hdr_t hdr;
} __attribute__ ((__packed__)) sstwr_init_msg_t;

/* Ranging responder message, embedding responder timestamps */
typedef struct {
  ieee154_hdr_t hdr;
  uint8_t init_rx_ts[RNG_TS_LEN];
  uint8_t resp_tx_ts[RNG_TS_LEN];
} __attribute__ ((__packed__)) sstwr_resp_msg_t;

/*---------------------------------------------------------------------------*/
/* Manage timestamps embedded in packets */

/* Obtain RX timestamp of the last packet received by the radio */
uint64_t get_rx_timestamp(void);

/* Obtain RX timestamp of the last packet transmitted by the radio */
uint64_t get_tx_timestamp(void);

/* Predict TX timestamp to embed it in the response.
 * When scheduling transmissions at a given time in the future,
 * scheduled TX time and actual TX time will not be exactly the same
 * due to limited TX time granularity and antenna delays.
 */
uint64_t predict_tx_timestamp(uint64_t tx_ts);

/* Embed timestamp in the specified field (init RX or resp TX) of response.
 * Used by responder.
 */
void resp_msg_set_timestamp(uint8_t *ts_field, uint64_t ts);

/* Read timestamp in the specified field (init RX or resp TX) of response.
 * Used by initiator.
 */
void resp_msg_get_timestamp(uint8_t *ts_field, uint64_t *ts);

/*---------------------------------------------------------------------------*/
/* Packet transmission and reception */

/* Set source, destination and sequence number in the pointed header */
void fill_ieee_hdr(ieee154_hdr_t *hdr, linkaddr_t *src, linkaddr_t *dst, uint8_t seqn);

/* Radio initialization */
void radio_init();

/* Packet transmission.
 * data: pointer to the data to be transmitted;
 * len: number of bytes of data;
 * mode: DWT_START_TX_IMMEDIATE, or DWT_START_TX_DELAYED for scheduled
 *      transmissions. Can be combined with DWT_RESPONSE_EXPECTED to activate
 *      the radio right after transmission if a reply is expeceted
 * rx_to: timeout in ~us for the radio to turn off if no packet is received.
 *      Ignored unless DWT_RESPONSE_EXPECTED is set.
 * tx_time: wanted TX time for the transmission (will differ from actual TX
 *      time - see predict_tx_timestamp()).
 */
uint8_t start_tx(void *data, uint8_t len, uint8_t mode, uint16_t rx_to, uint64_t tx_time);

/* Turn on the radio and enable reception */
void start_rx(uint16_t rx_to);

/* Wait until a packet is received, or an error/timeout occurs */
uint8_t wait_rx();

/* Upon reception, read pkt_size bytes into pkt */
uint8_t read_rx_data(void* pkt, uint32_t pkt_size);

/* Reset radio after errors and timeouts */
void radio_reset();
/*---------------------------------------------------------------------------*/
#endif /* RNG_SUPPORT_H */
