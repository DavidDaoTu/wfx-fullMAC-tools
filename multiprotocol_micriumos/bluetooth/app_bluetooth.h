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

#define INDICATION_TIMER_PERIOD_MSEC              1200    //FIXME issue with inferior values
#define INDICATION_TIMER_TIMEOUT_MSEC             1000

#define BLE_MASTER_NAME_MAX_LEN                   15
#define BLE_MASTER_CONNECTION                     1

#define BLE_STATE_CONNECTED       (1<<1)

#define DEVNAME_LEN 8        /**< Device name length (incl term null). */

/**************************************************************************//**
 * User-defined types & functions.
 *****************************************************************************/
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
#endif // APP_BLUETOOTH_H
