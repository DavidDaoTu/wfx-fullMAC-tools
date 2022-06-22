/***************************************************************************//**
 * @file
 * @brief User Interface for mp.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
//#ifndef DEMO_UI_H
//#define DEMO_UI_H

/**************************************************************************//**
* DEMO UI uses the underlying DMD interface and the GLIB and exposes several
* wrapper functions to application. These functions are used to display
* different bitmaps for the mp.
*
******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

typedef enum {
  MP_UI_PROTOCOL1,
  MP_UI_PROTOCOL2
} mpUIProtocol;

typedef enum {
  MP_UI_LIGHT_OFF,
  MP_UI_LIGHT_ON
} mpUILightState_t;

typedef enum {
  MP_UI_DIRECTION_PROT1,
  MP_UI_DIRECTION_PROT2,
  MP_UI_DIRECTION_SWITCH,
  MP_UI_DIRECTION_INVALID
} mpUILightDirection_t;

typedef enum {
  MP_UI_NO_NETWORK,
  MP_UI_SCANNING,
  MP_UI_JOINING,
  MP_UI_FORMING,
  MP_UI_NETWORK_UP,
  MP_UI_STATE_UNKNOWN
} mpUIZigBeeNetworkState_t;

/*******************************************************************************
 ******************************   PROTOTYPES   *********************************
 ******************************************************************************/

/**************************************************************************//**
 * @brief
 *   Initilize the GLIB and DMD interfaces.
 *
 * @param[in] void
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIInit(void);

/**************************************************************************//**
 * @brief
 *   Update the display with Silicon Labs logo and application name.
 *
 * @param[in] name name of the current application.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayHeader(uint8_t* name);

/**************************************************************************//**
 * @brief
 *   Update the display with Help menu.
 *
 * @param[in] networkForming whether UI displays network forming related components.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayHelp(bool networkForming);

/**************************************************************************//**
 * @brief
 *   Update the display with light bulb image.
 *
 * @param[in] on status of light bulb
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayLight(bool on);

/**************************************************************************//**
 * @brief
 *   Update the display to show if the bluetooth is connected to the mobile device.
 *
 * @param[in] bool, true if the Light is connected to mobile device, false otherwise.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayProtocol(mpUIProtocol protocol, bool isConnected);

/**************************************************************************//**
 * @brief
 *   Update the display to show which interface toggled the light.
 *
 * @param[in] DEMO_UI_DIRECTION_BLUETOOTH or DEMO_UI_DIRECTION_ZIGBEE
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayDirection(mpUILightDirection_t direction);

/**************************************************************************//**
 * @brief
 *   Update the display to clear signs used to indicate light toggle source.
 *
 * @param[in] DEMO_UI_DIRECTION_BLUETOOTH or DEMO_UI_DIRECTION_ZIGBEE
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIClearDirection(mpUILightDirection_t direction);

/**************************************************************************//**
 * @brief
 *   Update the display to show the device id.
 *
 * @param[in] protocol - DEMO_UI_PROTOCOL1 or DEMO_UI_PROTOCOL2 (left or right)
 * @param[in] id - id to show, max 9 byte long string.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayId(mpUIProtocol protocol, uint8_t* id);

/**************************************************************************//**
 * @brief
 *   Clear the Lcd screen and display the main screen.
 *
 * @param[in] name - application name
 * @param[in] showPROT1 - show protocol 1 related icon.
 * @param[in] showPROT2 - show protocol 2 related icon.
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIClearMainScreen(uint8_t* name, bool showPROT1, bool showPROT2);

/**************************************************************************//**
 * @brief
 *   Update the current operating channel.
 *
 * @param[in] channel
 *
 * @return
 *      void
 *****************************************************************************/
void mpUIDisplayChan(uint8_t channel);

//#endif //DEMO_UI_H
