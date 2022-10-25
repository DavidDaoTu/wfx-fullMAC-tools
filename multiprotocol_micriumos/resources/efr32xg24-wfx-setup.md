 # Set Up a EFR32xG24 Wireless Gecko Pro Kit and a WFx Wi-Fi® Expansion Kit
**NOTE**: The EFR32xG24 Radio Board can be used with either the Wireless Starter Kit Mainboard (BRD4001A) or the Wireless Pro Kit Mainboard (BRD4002A). This tutorial is written with respect to the BRD4001A (Starter Kit Mainboard).
 1. Connect the WFx Wi-Fi® Expansion Kit to the EFR32xG24 Wireless Gecko Starter Kit using jump wires as shown below:
 <img src=ssv5-xG24wiring-01.svg>
 

 Indeed the EFR32xG24 pin mapping on the Expander connector doesn't allow to directly connect the WFx Wi-Fi Expansion Kit to this connector and have the multiprotocol example working working with full features (LCD, VCOM, BLE, Wi-fi). This is due to the fact that several pins needed for the SPI to the WFx side are dedicatedly used for the LCD and VCOM through the radio connectors. To overcome this, jump wires and unused pins are used to reroute the connections as in the figure. 

 2. Make sure the two switches on the WF(M)200 expansion kit are on the correct position:

* "On Board LDO" for the power switch
* SPI for the bus switch

3. Connect the Silicon Labs Wireless Pro Kit baseboard to your PC using the USB cable. The board should appear as a device named "JLink Silicon Labs"

# Import the Multiprotocol Example Project into Simplicity Studio
> **Important Note**:
In order to run the Multiprotocol Example, we will need to modify the pin config (after done the wiring). We can either use the patch provided [here](../patches/brd4187/app.patch) or modify the pin config manually. Details are described below.

### **Method 1: Using the applicaton patch file, and then create the project**
This method is for quick demonstration, users don't need to manually configure components & its pins setings too much. Remember to apply the driver.patch file in gecko_sdk folder before doing this step.
1. From the root folder of the wifi application examples, open the command terminal and run the following command:
```
git apply --ignore-whitespace multiprotocol_micriumos/patches/brd4187/app.patch
``` 
or 
```
git apply --whitespace=fix multiprotocol_micriumos/patches/brd4187/app.patch
``` 
2. Open the Launcher in Simplicity Studio 5. It should recognize your connected devices. Click on **Start** button.
<img src=01-launcher-jlink.PNG>
3. Choose EXAMPLE PROJECTS & DEMOS, and click on the **CREATE** button. The "Project Configuration" dialog should appear. Rename the project if necessary and click on **FINISH**.
<img src=02-example-project-and-demos.PNG>

4. Now navigate to the folder multiprotocol_micriumos/patches/brd4187/config, and copy two config files:
```
sl_iostream_eusart_vcom_config.h
sl_wfx_host_bus_pinout.h
```

and overwrite existing files in the config folder of the newly created project:

<img src=overwrite_config.PNG>

5. Finally, build and run the project on the flying-wire connection xG24 board with enabled VCOM & LCD. 
>***Note***: Using this method, please do NOT open the pintool, otherwise Simplicity Studio 5 will automatically modify the config files we have just overwritten. If you do open the pintool, then make sure that all the configurations for **SL_IOSTREAM_EUSART_VCOM** is correct as manually done in Step#3 of **[Method 2](#method2)**.

6. After generating, building & running sucessfully project, we must reverse the applied xG24's app.patch file. This helps other boards can work.

```
git apply --ignore-whitespace -R multiprotocol_micriumos/patches/brd4187/app.patch
``` 
or 
```
git apply --whitespace=fix -R multiprotocol_micriumos/patches/brd4187/app.patch
``` 
### <a id="method2"></a>**Method 2: Create the project and configure the pins manually**
This method requires users have to install & configure pins manually.
1. Open the Launcher in Simplicity Studio 5. It should recognize your connected devices. Click on **Start** button.
<img src=01-launcher-jlink.PNG>
2. Choose EXAMPLE PROJECTS & DEMOS, and click on the **CREATE** button. The "Project Configuration" dialog should appear. Rename the project if necessary and click on **FINISH**.
<img src=02-example-project-and-demos.PNG>
3. After project creation, open the .slcp file to add components and configure the pinout:
* Enable Display: Open **SOFTWARE COMPONENTS**, search for **Board Control** and click **Configure**

<img src=03-board-control.png>

Then, slide the **Enable Display** to the right to activate

<img src=04-board-control-enable-display.png>

* Enable VCOM: search for the **IO Stream: EUSART** and click **Install**. Enter a name for it, e.g. vcom.

<img src=05-iostream-eusart-install.PNG>

After installing the component, click on **Configure**, scroll down to find section **SL_IOSTREAM_EUSART_VCOM**. Change the config as follows.

<img src=06-iostream-eusart-config.PNG>

Ignore the warning on TX, upcoming configurations will fix this conflict.
* Configure WFx Bus SPI: Search for **WFx Bus SPI** component and config the pins under **SL_WFX_HOST_PINOUT_SPI** and **SL_WFX_HOST_PINOUT_SPI_WIRQ**:
<img src=07-wfx-bus-spi.PNG>
* Configure the WFx FMAC driver: 
<img src=07.5-wfx-fmac-driver.PNG>
* Install **IO Stream: Retarget STDIO**
<img src=08-iostream-retarget-stdio.PNG>
* Install **UI Demo Functions**
<img src=09-ui-demo.PNG>
4. Build the Project and Run it on the flying-wired Kit. A working implementation is shown below:
<img src=implementation.jpg>
