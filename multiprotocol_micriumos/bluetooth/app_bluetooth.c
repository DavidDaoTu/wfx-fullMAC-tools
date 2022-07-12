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

static sl_bt_gatt_client_config_flag_t light_state_gatt_flag = sl_bt_gatt_disable;
static sl_bt_gatt_client_config_flag_t trigger_source_gatt_flag = sl_bt_gatt_disable;
static sl_bt_gatt_client_config_flag_t source_address_gatt_flag = sl_bt_gatt_disable;

static bool ble_indication_pending = false;
static bool ble_indication_ongoing = false;

static uint8_t ble_nb_connected = 0;
static uint8_t ble_state = 0;

static BleConn_t ble_conn[SL_BT_CONFIG_MAX_CONNECTIONS];
static bd_addr ble_own_addr = {0};
static char ble_own_name[DEVNAME_LEN] = {0};

// Timers for periodic & timeout indication
static sl_simple_timer_t app_bt_timers[2]; // periodic & timeout timer handles
static BleTimer_t g_timer_types[2] = {bleTimerIndicatePeriod, bleTimerIndicateTimeout};
static void app_single_timer_cb(sl_simple_timer_t *handle, void *data);


// return false on error
static bool bleConnAdd(uint8_t handle, bd_addr* address)
{
  for (uint8_t i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
    if (ble_conn[i].inUse == false) {
      ble_conn[i].handle = handle;
      memcpy((void*)&ble_conn[i].address, (void*)address, sizeof(ble_conn[i].address));
      ble_conn[i].inUse = true;
      return true;
    }
  }
  return false;
}

static bool bleConnRemove(uint8_t handle)
{
  for (uint8_t i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
    if (ble_conn[i].handle == handle) {
      ble_conn[i].handle = 0;
      memset((void*)&ble_conn[i].address.addr, 0, sizeof(ble_conn[i].address.addr));
      ble_conn[i].inUse = false;
      return true;
    }
  }
  return false;
}

static BleConn_t* bleConnGet(uint8_t handle)
{
  for (uint8_t i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
    if (ble_conn[i].handle == handle) {
      return &ble_conn[i];
    }
  }
  return NULL;
}

void bluetooth_app_request_send_indication (void)
{
  if (ble_nb_connected > 0) {
    ble_indication_pending = true;
  }
}


static int bluetooth_app_send_indication(uint16_t ble_characteristic)
{
  int res = -1;

  switch (ble_characteristic) {

    case LIGHT_STATE_GATTDB:
    {
      // stop light indication confirmation timer
      sl_simple_timer_stop(&app_bt_timers[bleTimerIndicateTimeout]);

      if (light_state_gatt_flag == sl_bt_gatt_indication) {
        uint8_t light_state = interface_light_get_state();

        // start timer for light indication confirmation
        sl_simple_timer_start(&app_bt_timers[bleTimerIndicateTimeout],
                              INDICATION_TIMER_TIMEOUT_MSEC,
                              app_single_timer_cb,
                              &g_timer_types[bleTimerIndicateTimeout],
                              false);

        /* Send notification/indication data */
        sl_bt_gatt_server_send_indication(ble_conn[0].handle,
                                           LIGHT_STATE_GATTDB,
                                           sizeof(light_state),
                                           (uint8_t*)&light_state);
        res = 0;
      }
      break;
    }

    case TRIGGER_SOURCE_GATTDB:
    {
      // stop light indication confirmation timer
      sl_simple_timer_stop(&app_bt_timers[bleTimerIndicateTimeout]);
      if (trigger_source_gatt_flag == sl_bt_gatt_indication) {
        uint8_t trigger = (uint8_t)interface_light_get_trigger();

        // start timer for trigger source indication confirmation
        sl_simple_timer_start(&app_bt_timers[bleTimerIndicateTimeout],
                              INDICATION_TIMER_TIMEOUT_MSEC,
                              app_single_timer_cb,
                              &g_timer_types[bleTimerIndicateTimeout],
                              false);

        /* Send notification/indication data */
        sl_bt_gatt_server_send_indication(ble_conn[0].handle,
                                         TRIGGER_SOURCE_GATTDB,
                                         sizeof(trigger),
                                         &trigger);
        res = 0;
      }
      break;
    }

    case SOURCE_ADDRESS_GATTDB:
    {
      // stop light indication confirmation timer
      sl_simple_timer_stop(&app_bt_timers[bleTimerIndicateTimeout]);
      if (source_address_gatt_flag == sl_bt_gatt_indication) {
        // start timer for source address indication confirmation
          sl_simple_timer_start(&app_bt_timers[bleTimerIndicateTimeout],
                                        INDICATION_TIMER_TIMEOUT_MSEC,
                                        app_single_timer_cb,
                                        &g_timer_types[bleTimerIndicateTimeout],
                                        false);

        uint8_t addr[8] = {0};
        interface_mac_t mac;

        // Retrieve the MAC address of the source which last triggered the light
        interface_light_get_mac_trigger(&mac);
        memcpy(addr, (uint8_t *)&mac, sizeof(mac));

        /* Send notification/indication data */
        sl_bt_gatt_server_send_indication(ble_conn[0].handle,
                                           SOURCE_ADDRESS_GATTDB,
                                           sizeof(addr),
                                           addr);
        res = 0;
      }
      break;
    }
  }

  return res;
}

/**************************************************************************//**
 * Acquire Light mutex
 * @param[in] handle timer handle
 * @param[in] data additional data
 *****************************************************************************/
static void app_single_timer_cb(sl_simple_timer_t *handle,
                                void *data)
{
  (void)handle;
  int res = -1;

  BleTimer_t *p_timer_type = data;
  int timer_type = (int) *p_timer_type;

  switch (timer_type) {

    /* Indication period reached */
    case bleTimerIndicatePeriod:      
      if (!ble_indication_ongoing) {
        ble_indication_ongoing = true;

        // Start sending BLE indications
        res = bluetooth_app_send_indication(LIGHT_STATE_GATTDB);
        if (res != 0) {
          // Error, no indications are sent as long as the remote GATT doesn't read the characteristics first.
          ble_indication_ongoing = false;
        }
      } else {
        ble_indication_pending = true;
      }
      break;

      /* Indication timeout */
    case bleTimerIndicateTimeout:
      ble_indication_ongoing = false;
      break;
   }
}

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

  int res = -1; 

  // Check if an indication is pending
  if (ble_indication_pending & !ble_indication_ongoing) {
    ble_indication_pending = false;
    ble_indication_ongoing = true;
    bluetooth_app_send_indication(LIGHT_STATE_GATTDB);
  }

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

      memcpy((void *)&ble_own_addr, (void *)&address, sizeof(ble_own_addr));
      // Update the LCD display with the BLE Id
      interface_display_ble_id((uint8_t *)&ble_own_addr.addr);
      snprintf(ble_own_name, DEVNAME_LEN, "MP%02X%02X",
               ble_own_addr.addr[1], ble_own_addr.addr[0]);
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
      ble_nb_connected++;
      ble_state |= BLE_STATE_CONNECTED;

      bleConnAdd(evt->data.evt_connection_opened.connection,
                 &evt->data.evt_connection_opened.address);

      printf("BLE: connection opened %d (%02X%02X%02X%02X%02X%02X)\r\n",
             evt->data.evt_connection_opened.connection,
             evt->data.evt_connection_opened.address.addr[5],
             evt->data.evt_connection_opened.address.addr[4],
             evt->data.evt_connection_opened.address.addr[3],
             evt->data.evt_connection_opened.address.addr[2],
             evt->data.evt_connection_opened.address.addr[1],
             evt->data.evt_connection_opened.address.addr[0]);

      if (ble_nb_connected == 1) {
          interface_display_ble_state(true);
          sl_simple_timer_start(&app_bt_timers[bleTimerIndicatePeriod],
                                INDICATION_TIMER_PERIOD_MSEC,
                                app_single_timer_cb,
                                &g_timer_types[bleTimerIndicatePeriod],
                                true);
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      ble_nb_connected--;
      bleConnRemove(evt->data.evt_connection_closed.connection);
      printf("BLE: connection closed, reason: %#02x\r\n", evt->data.evt_connection_closed.reason);

      if (ble_nb_connected == 0) {
        ble_state &= ~BLE_STATE_CONNECTED;
        interface_display_ble_state(false);

        /* No device connected, stop sending indications */
        sl_simple_timer_stop(&app_bt_timers[bleTimerIndicatePeriod]);
        sl_simple_timer_stop(&app_bt_timers[bleTimerIndicateTimeout]);
        ble_indication_ongoing = false;
        ble_indication_pending = false;
      }
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

      switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
        case LIGHT_STATE_GATTDB: {
          BleConn_t *connPoi = bleConnGet(evt->data.evt_gatt_server_user_write_request.connection);
          if (connPoi != NULL) {
            interface_light_set_state(interface_light_trigger_src_bluetooth,
                                      &connPoi->address,
                                      evt->data.evt_gatt_server_user_write_request.value.data[0]);
            
            /* Send response to write request */
            sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                        LIGHT_STATE_GATTDB,
                                                        0);
          }
          break;
        }
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

    /* This event indicates either that a local Client Characteristic Configuration descriptor
     * has been changed by the remote GATT client, or that a confirmation from the remote GATT
     * client was received upon a successful reception of the indication. */
    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
      
      switch (evt->data.evt_gatt_server_characteristic_status.characteristic) {

        case LIGHT_STATE_GATTDB:
        {
          switch ((sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {

            // confirmation of indication received from remote GATT client
            case sl_bt_gatt_server_confirmation:
              // Notification received from the remote GAT, send the next indication
              bluetooth_app_send_indication(TRIGGER_SOURCE_GATTDB);
              break;

              // client characteristic configuration changed by remote GATT client
            case sl_bt_gatt_server_client_config:
              light_state_gatt_flag = (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              break;
          }
          break;
        }

        case TRIGGER_SOURCE_GATTDB:
        {
          switch ((sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {

            // confirmation of indication received from remote GATT client
            case sl_bt_gatt_server_confirmation:
              // Notification received from the remote GAT, send the next indication
              bluetooth_app_send_indication(SOURCE_ADDRESS_GATTDB);
              break;

              // client characteristic configuration changed by remote GATT client
            case sl_bt_gatt_server_client_config:
              trigger_source_gatt_flag = (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              break;
          }
          break;
        }

        case SOURCE_ADDRESS_GATTDB:
        {
          switch ((sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {
            // confirmation of indication received from remote GATT client
            case sl_bt_gatt_server_confirmation:
              // All indications sent successfully, stop the timer
              sl_simple_timer_stop(&app_bt_timers[bleTimerIndicateTimeout]);

              ble_indication_ongoing = false;
              break;

              // client characteristic configuration changed by remote GATT client
            case sl_bt_gatt_server_client_config:
              source_address_gatt_flag = (sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags;
              break;
          }
          break;
        }
      }
      break;
    }    
    // Default event handler.
    default:
      break;
  }
}

uint8_t bluetooth_app_get_ble_state (void)
{
  return ble_state;
}

void bluetooth_app_get_own_mac (bd_addr *mac)
{
  if (mac != NULL) {
    memcpy(mac, &ble_own_addr, sizeof(bd_addr));
  }
}

int bluetooth_app_get_master_mac (bd_addr *mac)
{
  BleConn_t *conn;
  int ret = -1;

  if (mac != NULL) {
    conn = bleConnGet(BLE_MASTER_CONNECTION);
    if (conn != NULL) {
      memcpy(mac, &conn->address, sizeof(bd_addr));
      ret = 0;
    }
  }

  return ret;
}

void bluetooth_app_get_own_name (char *name, int name_size)
{
  if (name != NULL) {
    strncpy(name, ble_own_name, name_size);
  }
}

int bluetooth_app_get_master_name (char *name, int name_size)
{
  BleConn_t *conn;
  int ret = -1;

  if (name != NULL) {
    conn = bleConnGet(BLE_MASTER_CONNECTION);
    if (conn != NULL) {
      strncpy(name, conn->name, name_size);
      name[name_size-1] = '\0';
      ret = 0;
    }
  }

  return ret;
}
