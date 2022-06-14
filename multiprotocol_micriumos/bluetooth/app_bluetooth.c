/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "app_bluetooth.h"
#include "interface.h"
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      // Pad and reverse unique ID to get System ID.
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_general_discoverable,
        sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_general_discoverable,
        sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    /* This event indicates that a remote GATT client is attempting to write a value of an
    * attribute in to the local GATT database, where the attribute was defined in the GATT
    * XML firmware configuration file to have type="user".  */
    case sl_bt_evt_gatt_server_user_write_request_id:
    {
      printf("\r\n Inside user_write_request\r\n");
      switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
        case LIGHT_STATE_GATTDB:
        {
  //            BleConn_t *connPoi = bleConnGet(evt->data.evt_gatt_server_user_write_request.connection);
  //            if (connPoi != NULL) {
  //              interface_light_set_state(interface_light_trigger_src_bluetooth,
  //                                        &connPoi->address,
  //                                        evt->data.evt_gatt_server_user_write_request.value.data[0]);
            
          interface_light_set_state(interface_light_trigger_src_bluetooth,
                                    &evt->data.evt_connection_opened.address,
                                    evt->data.evt_gatt_server_user_write_request.value.data[0]);
            
            /* Send response to write request */
            sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                        LIGHT_STATE_GATTDB,
                                                        0);
          }
          break;
        }
      
      break;
    }
      
    /* This event indicates that a remote GATT client is attempting to read a value of an
      *  attribute from the local GATT database, where the attribute was defined in the GATT
      *  XML firmware configuration file to have type="user". */
    case sl_bt_evt_gatt_server_user_read_request_id:
    {   
    
      uint16_t sent_len = 0;
      switch (evt->data.evt_gatt_server_user_read_request.characteristic) 
      {

        /* Light state read */
        case LIGHT_STATE_GATTDB:
        {
          uint8_t light_state = interface_light_get_state();
          

          /* Send response to read request */
          sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                    LIGHT_STATE_GATTDB,
                                                    0,
                                                    sizeof(light_state),
                                                    (uint8_t*)&light_state,
                                                    &sent_len);
          break;
        }

        /* Trigger source read */
        case TRIGGER_SOURCE_GATTDB:
        {
          uint8_t trigger = (uint8_t)interface_light_get_trigger();

          /* Send response to read request */
          sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                        TRIGGER_SOURCE_GATTDB,
                                                        0,
                                                        sizeof(trigger),
                                                        &trigger,
                                                        &sent_len);
          break;
        }

        /* Source address read */
        case SOURCE_ADDRESS_GATTDB:
        {
          uint8_t addr[8] = {0};
          interface_mac_t mac;

          // Retrieve the MAC address of the source which last triggered the light
          interface_light_get_mac_trigger(&mac);
          memcpy(addr, (uint8_t *)&mac, sizeof(mac));

          /* Send response to read request */
          sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                        SOURCE_ADDRESS_GATTDB,
                                                        0,
                                                        sizeof(addr),
                                                        addr,
                                                        &sent_len);
          break;
        }
      }
      break;
    }
      
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
