# winc-update
Simplify updating firmware in the Microchip WINC1500 "WiFi System On Chip"
_-- R.D. Poor July 2022_

# About `winc-update`
The [Microchip WINC1500 "Wifi System On Chip"](https://www.microchip.com/en-us/product/ATWINC1500) is a versatile communication tool for building IoT devices.  It supports 802.15.4 b/g/n communication, it can act as an Access Point or as a Station, it provides socket-level communication and can standby at very low currents.

The WINC1500 has a long history, which also means that there have been many firmware updates.  To date, the most widely supported mechanism for updating the WINC firmware has been to use a dedicated SAMD21 processor, communicating via USB to a PC which in turn runs a suite of batch files.  This is somewhat complicated and requires a dedicated set of tools.

This `winc-update` repository takes a different approach.  It implements an application for the [SAME54 XPRO development system](https://www.microchip.com/en-us/development-tool/ATSAME54-XPRO) with [WINC1500 XPRO](https://www.microchip.com/en-us/development-tool/ATWINC1500-XPRO) and [IO1 XPRO](https://www.microchip.com/en-us/development-tool/ATIO1-XPRO) extension boards.  The application has three main functions:
* It can extract the contents of the WINC1500 firmware into a file on a microSD card
* It can update the WINC1500 firmware from a file on a microSD card.
* It can compare and verify the existing WINC1500 firmware against a file on a microSD card.

# Prerequisites

To run `winc-update` you will need:
* [SAME54 XPRO development system](https://www.microchip.com/en-us/development-tool/ATSAME54-XPRO)
* [WINC1500 XPRO](https://www.microchip.com/en-us/development-tool/ATWINC1500-XPRO)
* [IO1 XPRO](https://www.microchip.com/en-us/development-tool/ATIO1-XPRO)
* A PC or Laptop with a serial terminal program such as CoolTerm, TeraTerm or puTTY.
* A USB A to USB micro cable
* A microSD card, formatted for FATFS
* [Microchip MPLAB.X](https://www.microchip.com/en-us/tools-resources/develop/mplab-x-ide) or some means to load a .hex file into the SAME54

Connect the components as shown, with:
* the WINC XPRO on the EXT1 connector 
* the IO1 XPRO on EXT2
* the USB cable connecting the "Debug USB" port to your PC or laptop
* the microSD card plugged into the socket of the IO1 XPRO board

![Physical Setup](/docs/IMG_5907.jpg)
