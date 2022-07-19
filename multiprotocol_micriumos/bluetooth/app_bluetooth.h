#ifndef APP_BLUETOOTH_H
#define APP_BLUETOOTH_H

#include "interface.h"
#include "sl_bt_api.h"
#include "sl_simple_timer.h"
/**************************************************************************//**
 * DEFINES.
 *****************************************************************************/
#define LIGHT_STATE_GATTDB                        gattdb_light_state
#define TRIGGER_SOURCE_GATTDB                     gattdb_trigger_source
#define SOURCE_ADDRESS_GATTDB                     gattdb_source_address
#define DEV_NAME_GATDB                            gattdb_device_name

#define INDICATION_TIMER_PERIOD_MSEC              1200    //FIXME issue with inferior values
#define INDICATION_TIMER_TIMEOUT_MSEC             1000

#define BLE_GATT_SERVICE_GENERIC_ACCESS_UUID      0x1800
#define BLE_GATT_CHARACTERISTIC_DEVICE_NAME_UUID  0x2A00
#define BLE_MASTER_NAME_MAX_LEN                   15
#define BLE_MASTER_CONNECTION                     1

#define BLE_STATE_ADVERTISING                     (1<<0) 
#define BLE_STATE_CONNECTED                       (1<<1)
#define MAX_BUF_LEN                               128 

/**< Device name length (incl term null). */
#define DEVNAME_LEN                                8        
extern char boot_message[MAX_BUF_LEN];

/**************************************************************************//**
 * User-defined types & functions.
 *****************************************************************************/
typedef enum {
  get_gatt_service_handle = 0,
  get_gatt_name_characteristic_handle,
  get_gatt_name_characteristic_value,
} ble_name_retrieval_step_t;

typedef enum {
  bleTimerIndicatePeriod    = 0,
  bleTimerIndicateTimeout   = 1,
} BleTimer_t;

typedef struct {
  uint8_t handle;
  bd_addr address;
  bool    inUse;
  uint8_t indicated;
  char    name[BLE_MASTER_NAME_MAX_LEN+1];
} BleConn_t;

void bluetooth_app_request_send_indication(void);
uint8_t bluetooth_app_get_ble_state (void);
void bluetooth_app_get_own_mac(bd_addr *mac);
int  bluetooth_app_get_master_mac(bd_addr *mac);
void bluetooth_app_get_own_name(char *name, int name_size);
int  bluetooth_app_get_master_name(char *name, int name_size);
void bluetooth_app_disconnect_master(void);
#endif // APP_BLUETOOTH_H
