#pragma once

/**
   @file

   Socket CAN driver (Linux).

   This driver requires the libsocketcan library.
*/

#include <csp/interfaces/csp_if_can.h>



/**
   Open CAN socket and add CSP interface.

   @param[in] device CAN device name (Linux device).
   @param[in] ifname CSP interface name, use #CSP_IF_CAN_DEFAULT_NAME for default name.
   @param[in] bitrate if different from 0, it will be attempted to change the bitrate on the CAN device - this may require increased OS privileges.
   @param[in] node_id CSP address of the interface.
   @param[in] promisc if \a true, receive all CAN frames. If \a false a filter is set on the CAN device, using device->addr
   @param[out] return_iface the added interface.
   @return The added interface, or NULL in case of failure.
*/
int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, int bitrate, bool promisc, csp_iface_t ** return_iface);

/**
   Initialize socketcan and add CSP interface.

   @deprecated version 1.6, use csp_can_socketcan_open_and_add_interface()
   @param[in] device CAN device name (Linux device).
   @param[in] node_id CSP address of the interface.
   @param[in] bitrate if different from 0, it will be attempted to change the bitrate on the CAN device - this may require increased OS privileges.
   @param[in] promisc if \a true, receive all CAN frames. If \a false a filter is set on the CAN device, using device->addr
   @return The added interface, or NULL in case of failure.
*/
csp_iface_t * csp_can_socketcan_init(const char * device, unsigned int node_id, int bitrate, bool promisc);

/**
   Stop the Rx thread and free resources (testing).

   @note This will invalidate CSP, because an interface can't be removed. This is primarily for testing.

   @param[in] iface interface to stop.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_can_socketcan_stop(csp_iface_t * iface);

