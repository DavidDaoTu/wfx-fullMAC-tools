

#include "interface.h"
#include "sl_simple_led_instances.h"

static uint8_t led_state = 0;
static interface_light_trigger_src_t led_trigger_source = interface_light_trigger_src_button;
static interface_mac_t mac_trigger = {0};

void interface_light_off (interface_light_trigger_src_t trigger,
                          interface_mac_t *mac)
{
  // Turn Off the LEDs
  sl_led_turn_off(&sl_led_led0);
  sl_led_turn_off(&sl_led_led1);

//  apply_light_change(trigger, mac, 0);
  led_state = 0;
}

void interface_light_on (interface_light_trigger_src_t trigger,
                          interface_mac_t *mac)
{
  // Turn On the LEDs
  sl_led_turn_on(&sl_led_led0);
  sl_led_turn_on(&sl_led_led1);
  
  
//  apply_light_change(trigger, mac, 0);
  led_state = 1;
}

void interface_light_set_state (interface_light_trigger_src_t trigger,
                                interface_mac_t *mac,
                                uint8_t new_led_state)
{
  if (new_led_state != 0) {
    interface_light_on(trigger, mac);
  } else {
    interface_light_off(trigger, mac);
  }
}

uint8_t interface_light_get_state (void)
{
  return led_state;
}

interface_light_trigger_src_t interface_light_get_trigger (void)
{
  return led_trigger_source;
}

void interface_light_get_mac_trigger (interface_mac_t *mac)
{
  if (mac != NULL) {
    memcpy(mac, &mac_trigger, sizeof(interface_mac_t));
  }
}