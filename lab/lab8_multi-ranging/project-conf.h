#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* use the following to optimise ranging delays for EVB1000 */
//#define DW1000_CONF_EXTREME_RNG_TIMING 1

#define APP_RADIO_CONF 3

#if APP_RADIO_CONF == 1
#define DW1000_CONF_CHANNEL        2
#define DW1000_CONF_PRF            DWT_PRF_64M
#define DW1000_CONF_PLEN           DWT_PLEN_128
#define DW1000_CONF_PAC            DWT_PAC8
#define DW1000_CONF_SFD_MODE       0
#define DW1000_CONF_DATA_RATE      DWT_BR_6M8
#define DW1000_CONF_PHR_MODE       DWT_PHRMODE_STD
#define DW1000_CONF_PREAMBLE_CODE  9
#define DW1000_CONF_SFD_TIMEOUT    (129 + 8 - 8)
// #define DW1000_CONF_RX_ANT_DLY     (16496)
// #define DW1000_CONF_TX_ANT_DLY     (16496)

#elif APP_RADIO_CONF == 2
#define DW1000_CONF_CHANNEL        4
#define DW1000_CONF_PRF            DWT_PRF_64M
#define DW1000_CONF_PLEN           DWT_PLEN_128
#define DW1000_CONF_PAC            DWT_PAC8
#define DW1000_CONF_SFD_MODE       0
#define DW1000_CONF_DATA_RATE      DWT_BR_6M8
#define DW1000_CONF_PHR_MODE       DWT_PHRMODE_STD
#define DW1000_CONF_PREAMBLE_CODE  17
#define DW1000_CONF_SFD_TIMEOUT    (129 + 8 - 8)
// #define DW1000_CONF_RX_ANT_DLY     (16455)
// #define DW1000_CONF_TX_ANT_DLY     (16455)

#elif APP_RADIO_CONF == 3
#define DW1000_CONF_CHANNEL        5
#define DW1000_CONF_PRF            DWT_PRF_64M
#define DW1000_CONF_PLEN           DWT_PLEN_128
#define DW1000_CONF_PAC            DWT_PAC8
#define DW1000_CONF_SFD_MODE       0
#define DW1000_CONF_DATA_RATE      DWT_BR_6M8
#define DW1000_CONF_PHR_MODE       DWT_PHRMODE_STD
#define DW1000_CONF_PREAMBLE_CODE  9
#define DW1000_CONF_SFD_TIMEOUT    (129 + 8 - 8)
// #define DW1000_CONF_RX_ANT_DLY     (16436)
// #define DW1000_CONF_TX_ANT_DLY     (16436)

#else
#error App: radio config is not set
#endif

#define IEEE802154_CONF_PANID 0xA0B0
#define DW1000_CONF_FRAMEFILTER 1

#endif /* PROJECT_CONF_H_ */
