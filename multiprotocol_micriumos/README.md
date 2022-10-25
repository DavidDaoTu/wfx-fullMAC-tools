# Multiprotocol Micrium OS Example

The purpose of this application is to provide a Micrium OS example using multiple protocols to do something as simple as toggling LEDs.

BLE and Wi-Fi protocols are used in this example to communicate with it, **neither the Dynamic Multiprotocol feature (DMP) nor a coexistence mechanism are used**.
The BLE interface is used to send advertisements/beacons allowing to connect to the device and toggle the LEDs via the EFR Connect BLE Mobile App.
A Wi-Fi SoftAP interface is also provided by the device allowing to connect and access a Web Page and toggle the LEDs from it for instance.

## Requirements

### Hardware Prerequisites

One of the supported platforms listed below is required to run the example:

* [**EFR32xG21 Wireless Gecko Starter Kit (SLWSTK6006A)**](https://www.silabs.com/products/development-tools/wireless/efr32xg21-wireless-starter-kit),
  [**EFR32™ Mighty Gecko Wireless Starter Kit (SLWSTK6000B)**](https://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit), and [**EFR32xG24 Wireless Gecko Pro Kit**](https://www.silabs.com/development-tools/wireless/efr32xg24-pro-kit-20-dbm)
* [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)

Additionally, this example requires:

* a PC to configure the board, load a binary file on the board, compile the Simplicity Studio project, visualize the traces or access the Web Page
* a Smartphone to interact with the BLE interface and access the Web Page

### Software Prerequisites

* The required software includes Simplicity Studio v5, the Gecko SDK Suite (32-bit MCU and lwIP) and the Bluetooth SDK
* The example project and the Wi-Fi Full MAC driver (available in the Gecko Platform SDK)
* The **EFR Connect BLE Mobile App** available on [**Google Play**](https://play.google.com/store/apps/details?id=com.siliconlabs.bledemo&hl=en&gl=US) and [**App Store**](https://apps.apple.com/us/app/efr-connect-ble-mobile-app/id1030932759)
* A Serial terminal to communicate with the board. For example, [**Tera Term**](https://osdn.net/projects/ttssh2/releases/) or [**Putty**](https://www.putty.org/)
* A Web browser 

## Install Simplicity Studio 5 and the Gecko SDK

Simplicity Studio 5 is a free software suite needed to start developing your application. To install Simplicity Studio 5, please follow this [**procedure**](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-getting-started/install-ss-5-and-software) by selecting the options **[Install by connecting device(s)]** and **[Auto]**.

For EFR32xG21, EFR32xG24 boards, a driver patch is required because our updated fullMAC driver will be only released at the end of 2022. The patch can be found [**here**](patches/driver.patch). To apply the patch, simply put the driver.patch file in the gecko_sdk or wfx-fmac-driver (version 3.5.0) folder and go to that folder then run:
```
git apply --ignore-whitespace driver.patch
```
or 
```
git apply --whitespace=fix driver.patch
``` 

After testing, users can reverse the applied driver patch in gecko_sdk to the default state by:
```
git apply --ignore-whitespace -R driver.patch
```
or 
```
git apply --whitespace=fix -R driver.patch
```

## Set Up your Kit
> **Note**: For EFR32xG24 boards, we support both plug-and-play (default) and flying-wires connection modes. In the plug-and-play default mode, LCD and VCOM components are unavailable. In flying-wires connection mode, users must install & manually change configurations in order to enable all features (VCOM & LCD) of the multiprotocol application.

Please follow the instructions related to the platform suiting your case:
* [**EFR32xG24 Wireless Gecko Pro Kit setup**](resources/efr32xg24-wfx-setup.md) (For plug-and-play default mode, users can skip this setup guidelines)
* [**EFR32xG21 Wireless Gecko Starter Kit setup**](resources/efr32xg21-wfx-setup.md)

## Start the Example

1. Once the binary file transferred, the example starts sending BLE advertisements/beacons, launches a SoftAP interface named _**multiprotocol_softap**_
and displays the LED and the interface connection states on the LCD screen of the board (except for EFR32xG21 devices and EFR32xG24 devices in plug-and-play mode)
2. Enable the Bluetooth on your smartphone
3. Launch the EFR Connect BLE Mobile App and start the **Connected Lighting Demo** under the **Demo** view.
4. Select your BLE platform from the list of detected devices, it should be named **MPxxxx**.
If several devices are displayed, the two last bytes of the BLE MAC address, composing the BLE name, can be retrieved from the log traces displayed at boot
5. Your smartphone is now connected to the device using the Bluetooth interface.
Two dots are displayed on each side of the Bluetooth logo on the LCD screen to indicate this state of connection.
6. Touch the light bulb displayed by the application on the smartphone and watch the board LEDs toggle.
The LED state is also toggled on the LCD screen and an arrow appears shortly next to the Bluetooth logo to indicate the source of the light toggle. This arrow disappears after a second.
7. Enable the Wi-Fi on your smartphone
8. Connect to the Access Point _**multiprotocol_softap**_ provided by the device, with the password **changeme**.
9. Open a Web browser on your smartphone and go to [http://10.10.0.1/](http://10.10.0.1/). If a popup appears due to a lack of Internet connectivity on the Wi-Fi interface
make sure to request to stay connected, otherwise the Web page may not be displayed in your browser. 
1.  The displayed Web page not only gives the possibility to toggle the LEDs as the EFR Connect BLE Mobile App but it also allows to:
    * Shutdown an active BLE connection
    * Enable/disable the BLE advertisements
    * Shutdown active connections to the Wi-Fi Softap
    * Disable the Wi-Fi Softap interface
    * Scan and connect to Access Points surrounding the device
    * Display various information about the BLE and Wi-Fi interfaces and their connections
2.  LEDs can also be toggled from the push buttons PB0 & PB1 respectively on the board (except for EFR32xG21 devices)

This example is actually a combination of two already existing examples, for more information about the EFR Connect BLE Mobile App interactions, please refer to the [DMP Light Demo](https://docs.silabs.com/bluetooth/3.2/miscellaneous/mobile/efr-connect-mobile-app) and for more information about the Web page interactions, please refer to the [Wi-Fi Commissioning Example](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/wgm160p/wifi-commissioning-micriumos/interacting-with-the-example).
