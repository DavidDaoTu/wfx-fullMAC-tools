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

#define BLE_STATE_CONNECTED       (1<<1)
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

#endif // APP_BLUETOOTH_H
