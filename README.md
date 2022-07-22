# winc-update
A utility for updating firmware in the Microchip WINC1500 WiFi System On Chip
_-- R.D. Poor July 2022_

The [Microchip WINC1500 "Wifi System On Chip"](https://www.microchip.com/en-us/product/ATWINC1500) is a versatile communication tool for building IoT devices.  It supports 802.15.4 b/g/n communication, it can act as an Access Point or as a Station, it provides socket-level communication and can standby at very low currents.

The WINC1500 has a long history, which also means that there have been many firmware updates.  To date, the most widely supported mechanism for updating the WINC firmware has been to use a dedicated SAMD21 processor, communicating via USB to a PC which in turn runs a suite of batch files.  This is somewhat complicated and requires a dedicated set of tools.

This `winc-update` repository takes a different approach.  It implements an application for the SAME54 XPRO development system with WINC1500 XPRO and IO1 XPRO extension boards.  The application has three main functions:
* It can extract the contents of the WINC1500 firmware into a file on a microSD card
* It can update the WINC1500 firmware from a file on a microSD card.
* It can compare and verify the existing WINC1500 firmware against a file on a microSD card.
