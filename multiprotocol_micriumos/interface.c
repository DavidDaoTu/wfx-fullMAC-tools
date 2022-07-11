#include "interface.h"
#include "sl_simple_led_instances.h"
#include "mp-ui.h"

static uint8_t led_state = 0;
static interface_light_trigger_src_t led_trigger_source = interface_light_trigger_src_button;
static interface_mac_t mac_trigger = {0};
static interface_mac_t own_mac = {0};

static void apply_light_change(interface_light_trigger_src_t trigger,
                               interface_mac_t *mac,
                               uint8_t new_state);

void interface_light_off (interface_light_trigger_src_t trigger,
                          interface_mac_t *mac)
{
  // Turn Off the LEDs
  sl_led_turn_off(&sl_led_led0);
  sl_led_turn_off(&sl_led_led1);

  apply_light_change(trigger, mac, 0);
}

void interface_light_on (interface_light_trigger_src_t trigger,
                          interface_mac_t *mac)
{
  // Turn On the LEDs
  sl_led_turn_on(&sl_led_led0);
  sl_led_turn_on(&sl_led_led1);
  
  
  apply_light_change(trigger, mac, 1);  
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

void interface_display_ble_state (bool connected)
{

  mpUIDisplayProtocol(MP_UI_PROTOCOL2, connected);

}

void interface_display_wifi_state (bool connected)
{

  mpUIDisplayProtocol(MP_UI_PROTOCOL1, connected);

}

void interface_display_ble_id (uint8_t *id)
{

  char dev_id[9];
  // Only 5 characters are displayed correctly when the light is on
  sprintf(dev_id, "    %02X%02X", id[1], id[0]);
  mpUIDisplayId(MP_UI_PROTOCOL2, (uint8_t *)dev_id);

  // Save our own MAC as reference
  memcpy(&own_mac, id, sizeof(own_mac));
  memcpy(&mac_trigger, id, sizeof(mac_trigger));
}

void interface_display_wifi_id (uint8_t *id)
{
  mpUIDisplayId(MP_UI_PROTOCOL1, id);
}

static void apply_light_change (interface_light_trigger_src_t trigger,
                                interface_mac_t *mac,
                                uint8_t new_state)
{
  // Update the LCD
  mpUIDisplayLight(new_state);
  
  // The timer was running, a direction needs to be cleared !
  if (led_trigger_source == interface_light_trigger_src_wifi) {
    mpUIClearDirection(MP_UI_DIRECTION_PROT1);
  } else if (led_trigger_source == interface_light_trigger_src_bluetooth) {
    mpUIClearDirection(MP_UI_DIRECTION_PROT2);
  }  

  if (trigger == interface_light_trigger_src_wifi) {
    mpUIDisplayDirection(MP_UI_DIRECTION_PROT1);
  } else if (trigger == interface_light_trigger_src_bluetooth) {
    mpUIDisplayDirection(MP_UI_DIRECTION_PROT2);
  }

  // Send a BLE indication to update the application display
  bluetooth_app_request_send_indication();

  // Store data
  led_state = new_state;
  led_trigger_source = trigger;
  if (mac != NULL) {
    memcpy(&mac_trigger, mac, sizeof(mac_trigger));
  } else {
    // Set our own MAC in this case
    memcpy(&mac_trigger, &own_mac, sizeof(mac_trigger));
  }
}

void interface_init(void){
  mpUIInit();
  mpUIClearMainScreen((uint8_t *)"Multiprotocol", true, true);
}