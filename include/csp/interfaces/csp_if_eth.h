#pragma once

/**
   @file

   Ethernet (ETH) interface.

   Ethernet frames holds up to 1500 bytes, and may be limited by an MTU value that is lower, so in order to transmit CSP 
   packets larger than this, a segmentation protocol is required. The Ethernet standard assumes that this is handled by
   an upper lat protocol, like bt the IP protocol.
   This Ethernet driver therefore implements a CSP segmentation protocol that is called \b EFP. It is similar to CAN CFP.  

   Each packet segment is prepended an \b EFP header with the following content: 

   - Version:           1 bit
   - Unused:            2 bit
   - SegmentId:         5 bit
   - PacketId:          8 bit
   - PacketLengthMSB:   8 bit
   - PacketLengthLSB:   8 bit

   The \b Version is zero. EFP discard Ethernet frames with the leading bit being 1. For a potential new protocol to 
   coexist with EFP the first bit in the meader must be 1. 
   The \b Unused bits must be zero.  
   The \b SegmentId is a sequential number that starts at zero for each packet and increments by one for each segment. 
   It wraps around to zero at 32, implying that the larges CSP packet size that can be handled is 32 * MTU.
   The \b PacketId is a sequential number that is same for all segments of the same packet, and increments by one
   for every new packet. It wraps around to zero at 256, implying that the protocol can handle up to 256 packets
   that are processed in parallel.
   The \b PacketLengthMSB and \b PacketLengthLSB holds the packet length most and least significant bits.

   The protocol assumes that a single segment is sent per Ethernet frame. If smaller than the Ethernet payload length
   zero-padding is applied. Padding may only occur when transferring the last segment.
   The protocol allows for segments being received out of order.

   The protocol differs from CAN CFP by not explicitly storing CSP source and destination addresses in the EFP header.
   The packets are processed individually, not per CSP connection.  
*/

#include <csp/csp_interface.h>


#if 0
/**
   @defgroup EFP_SIZE EFP header size.
   @{
*/
/** Version - EFP protocol version. */
#define EFP_VERSION_SIZE    1
/** Unused */
#define EFP_UNUSED_SIZE     2
/** SegmentId - Identifier for the segment. */
#define EFP_SEGID_SIZE      5
/** PacketId - Identifier for the packet */
#define EFP_PKTID_SIZE      8
/** PacketLength MSB - The packet length most significant byte */
#define EFP_PKTLEN_MSB_SIZE    8
/** PacketLength LSB - The packet length least significant byte */
#define EFP_PKTLEN_LSB_SIZE    8
/** @} */

/**
   @defgroup EFP_FIELDS Macros for extracting fields from EFP header.
   @{
*/
/** Helper macro */
#define EFP_FIELD(id,rsiz,fsiz) ((uint32_t)((uint32_t)((id) >> (rsiz)) & (uint32_t)((1 << (fsiz)) - 1)))
/** Extract version */
#define EFP_VERSION(id) EFP_FIELD(id, EFP_UNUSED_SIZE + EFP_SEGID_SIZE + EFP_PKTID_SIZE + EFP_PKTLEN_MSB_SIZE + EFP_PKTLEN_LSB_SIZE, EFP_VERSION_SIZE)
/** Extract segment identifier */
#define EFP_SEGID(id) EFP_FIELD(id, EFP_PKTID_SIZE + EFP_PKTLEN_MSB_SIZE + EFP_PKTLEN_LSB_SIZE, EFP_SEGID_SIZE)
/** Extract packet identifier */
#define EFP_PKTID(id) EFP_FIELD(id, EFP_PKTLEN_MSB_SIZE + EFP_PKTLEN_LSB_SIZE, EFP_PKTID_SIZE)
/** Extract packet length most significant byte */
#define EFP_PKTLEN_MSB(id) EFP_FIELD(id, EFP_PKTLEN_LSB_SIZE, EFP_PKTLEN_MSB_SIZE)
/** Extract packet length least significant byte */
#define EFP_PKTLEN_LSB(id) EFP_FIELD(id, 0, EFP_PKTLEN_LSB_SIZE)
/** Extract packet length uint16_t value */
#define EFP_PKTLEN(id) ((uint16_t)256) * (EFP_PKTLEN_MSB(id) + EFP_PKTLEN_LSB(id))
/** @} */

/**
   @defgroup EFP_MAKE Macros for building EFP header.
   @{
*/
/** Helper macro */
#define EFP_MAKE_FIELD(id,fsiz,rsiz) ((uint32_t)(((id) & (uint32_t)((uint32_t)(1 << (fsiz)) - 1)) << (rsiz)))
/** Make version */
#define EFP_MAKE_VERSION(id) EFP_MAKE_FIELD(id, EFP_VERSION_SIZE, EFP_UNUSED_SIZE + EFP_SEGID_SIZE + EFP_PKTID_SIZE + EFP_PKTLEN_MSB_SIZE + EFP_PKTLEN_LSB_SIZE)
/** Make segment identifier */
#define EFP_MAKE_SEGID(id) EFP_MAKE_FIELD(id, EFP_SEGID_SIZE, EFP_PKTID_SIZE + EFP_PKTLEN_MSB_SIZE + EFP_PKTLEN_LSB_SIZE)
/** Make packet identifier */
#define EFP_MAKE_PKTID(id) EFP_MAKE_FIELD(id, EFP_PKTID_SIZE, EFP_PKTLEN_MSB_SIZE + EFP_PKTLEN_LSB_SIZE)
/** Make packet length most significant byte */
#define EFP_MAKE_PKTLEN_MSB(id) EFP_MAKE_FIELD(id, EFP_PKTLEN_MSB_SIZE, EFP_PKTLEN_LSB_SIZE)
/** Make packet length least significant byte */
#define EFP_MAKE_PKTLEN_LSB(id) EFP_MAKE_FIELD(id, EFP_PKTLEN_LSB_SIZE, 0)
/** Make packet length uint16_t value */
#define EFP_MAKE_PKTLEN(id) EFP_PKTLEN_MSB(id / 256) + EFP_PKTLEN_LSB(id % 256)
/** Make full segment identifier. Observe that version is implicitly set to zero. */
#define EFP_MAKE_HEADER(segid, pktid, pktlen) EFP_SEGID(segid) + EFP_PKTID(pktid) + EFP_PKTLEN(pktlen)
/** @} */

/** Mask to separate packets, masking all but the packet id */
#define EFP_ID_CONN_MASK  (EFP_MAKE_PKTID((uint32_t)(1 << EFP_PKTID_SIZE) - 1))

/** Conditions for first, following and last segment */
#define EFP_NUM_SEGS_HELP(id, efp_mtu) (uint32_t)(EFP_PKTLEN(id) / (efp_mtu) + (EFP_PKTLEN(id) % (efp_mtu) ? 1 : 0))
#define EFP_NUM_SEGS(id, mtu) EFP_NUM_SEGS_HELP(id, mtu - sizeof(uint32_t) - 2)
#define EFP_REMAIN(id, mtu) (uint32_t)(EFP_NUM_SEGS(id, mtu) - EFP_SEGID(id) - 1)
#define EFP_BEGIN(id) (EFP_SEGID(id))
#define EFP_LAST(id, mtu) (EFP_SEGID(id) == (EFP_NUM_SEGS(id, mtu) - 1))
#define EFP_SEG_OFFSET(id, mtu) (EFP_SEGID(id) * (mtu - sizeof(uint32_t) - 2))
#endif

/**
   Default interface name.
*/
#define CSP_IF_ETH_DEFAULT_NAME "ETH"

// 0x88B5 : IEEE Std 802 - Local Experimental Ethertype
#define ETH_TYPE_CSP 0x88B5

/* Max number of payload bytes per ETH frame */
#define ETH_FRAME_SIZE_MAX 1500

/**
   Send ETH frame (implemented by driver).

   Used by csp_eth_tx() to send Ethernet frames.

   @param[in] driver_data driver data from #csp_iface_t
   @param[in] data Ethernet payload data 
   @param[in] data_size size of \a data.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
typedef int (*csp_eth_driver_tx_t)(void * driver_data, const uint8_t * data, size_t data_size);

/**
   Interface data (state information).
*/
typedef struct {
    /* Signal to CSP functions */
    bool init_done;

    /*** Ethernet MAC address */
    uint8_t mac_addr[6];

    /** Ethernet type (protocol) field */
    uint16_t eth_type;

    /** EFP max frame size, which is the Ethernet MTU */
    uint16_t mtu;

    /** Packet id that is sanme for all segnments of same packet. 
     * Must be uniqueue among currently processed packets from given CSP source address. 
     */
    uint8_t packet_id;

    /** Tx function */
    csp_eth_driver_tx_t tx_func;
    /** PBUF queue */
    csp_packet_t * pbufs;
} csp_eth_interface_data_t;

/**
   Add interface.

   @param[in] iface CSP interface, initialized with name and inteface_data pointing to a valid #csp_eth_interface_data_t structure.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_eth_add_interface(csp_iface_t * iface);

void csp_if_eth_dma_rx_callback(void * pbuf, size_t pbuf_size);

void csp_if_eth_rx_loop(csp_iface_t * iface);
