#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <stdbool.h>
#include <stddef.h>
#include "sl_bt_types.h" 

typedef bd_addr interface_mac_t;

typedef enum {
  interface_light_trigger_src_button     = 0,
  interface_light_trigger_src_bluetooth  = 1,
  interface_light_trigger_src_wifi       = 2,
} interface_light_trigger_src_t;

void interface_light_off(interface_light_trigger_src_t trigger,
                         interface_mac_t *mac);

void interface_light_on(interface_light_trigger_src_t trigger,
                        interface_mac_t *mac);

void interface_light_set_state(interface_light_trigger_src_t trigger,
                               interface_mac_t *mac,
                               uint8_t new_led_state);
uint8_t interface_light_get_state(void);
void interface_light_get_mac_trigger (interface_mac_t *mac);
interface_light_trigger_src_t interface_light_get_trigger (void);

void interface_display_ble_state(bool connected);
void interface_display_wifi_state(bool connected);
void interface_display_ble_id(uint8_t *id);
void interface_display_wifi_id(uint8_t *id);
#endif /* INTERFACE_H_ */
