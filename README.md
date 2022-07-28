# winc-cloner
Simplify updating firmware in the Microchip WINC1500 "WiFi System On Chip"
_-- R.D. Poor July 2022_

# About `winc-cloner`
The [Microchip WINC1500 "Wifi System On Chip"](https://www.microchip.com/en-us/product/ATWINC1500) is a versatile communication tool for building IoT devices.  It supports 802.15.4 b/g/n communication, it can act as an Access Point or as a Station, it provides socket-level communication and can standby at very low currents.

The WINC1500 has a long history, which also means that there have been many firmware updates.  To date, the most widely supported mechanism for updating the WINC firmware has been to use a dedicated SAMD21 processor, communicating via USB to a PC which in turn runs a suite of batch files.  This is somewhat complicated and requires a dedicated set of tools.

This `winc-cloner` repository takes a different approach.  It implements an application for the [SAME54 XPRO development system](https://www.microchip.com/en-us/development-tool/ATSAMe54_xpro) with [WINC1500 XPRO](https://www.microchip.com/en-us/development-tool/ATWINC1500-XPRO) and [IO1 XPRO](https://www.microchip.com/en-us/development-tool/ATIO1-XPRO) extension boards.  `winc-cloner` has three main functions:
* It can extract the contents of the WINC1500 firmware into a file on a microSD card
* It can update the WINC1500 firmware from a file on a microSD card.
* It can compare and verify the existing WINC1500 firmware against a file on a microSD card.

# Prerequisites

To run `winc-cloner` you will need:
* [SAME54 XPRO development system](https://www.microchip.com/en-us/development-tool/ATSAMe54_xpro)
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

# Running `winc-cloner`

On your laptop or PC, launch your serial terminal emulator and connect it to
the serial port corresponding to the SAME54's EDBG output.  Set the terminal
emulator to 115200 baud, 8n1 framing.

Use MPLAB or the tool of your choice to load the `winc-cloner` firmware
into the SAME54.

When the program runs, you will see something like this:
```
####################
# winc-cloner v0.0.7 (https://github.com/rdpoor/winc-cloner)
####################

Found 2 files
   m2m_aio_3a0_v19_5_4.img
   m2m_aio_3a0_v19_7_7.img
Commands:
h: print this help
e: extract WINC firmware to a file
u: update WINC firmware from a file
c: compare WINC firmware against a file
r: recompute / rebuild WINC PLL tables
> 
```
At this point, you can type:
## `h` for help
This will simply repeat the help instructions.
## `e` to extract the WINC firmware to a file
For example:
```
Commands:
h: print this help
e: extract WINC firmware to a file
u: update WINC firmware from a file
c: compare WINC firmware against a file
>
Extract WINC firmware into filename: test.img
Extracting WINC firmware into test.img
Chip ID 1503a0
................................................................................................................................................................................................................................................................
Successfully extracted WINC contents into test.img
```
Each '.' represents one sector of data (FLASH_SECTOR_SZ) read from the WINC and
written to `test.img`.
## `u` to update the WINC firmware from a file
For example:
```
Commands:
h: print this help
e: extract WINC firmware to a file
u: update WINC firmware from a file
c: compare WINC firmware against a file
>
Update WINC firmware from filename: m2m_aio_3a0_v19_5_4.img
Updating WINC firmware from m2m_aio_3a0_v19_5_4.img
Chip ID 1503a0
Flash Size 8 Mb
=!=x=====!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!=!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!================================================================================================================================
Successfully updated WINC contents from m2m_aio_3a0_v19_5_4.img
```
A '=' indicates a sector that is identical in the file and in the WINC; these
sectors are left untouched.  Each '!' represents a sector that is erased in the
WINC memory and then written from the file data.  And 'x' represents a sector
that is skipped -- in this case, winc-cloner will not overwrite the gain or
pll tables of your existing WINC firmware.
## `c` to compare the WINC firmware against a file
For example:
```
Commands:
h: print this help
e: extract WINC firmware to a file
u: update WINC firmware from a file
c: compare WINC firmware against a file
>
Compare WINC firmware against filename: m2m_aio_3a0_v19_7_7.img
Comparing WINC firmware against m2m_aio_3a0_v19_7_7.img
Chip ID 1503a0
.
WINC and file differ at sector 0x1000.......
WINC and file differ at sector 0x9000
WINC and file differ at sector 0xa000
etc...
```

## Other Notes
The images/ directory of this repository contains some "All In One" WINC images,
currently including:
* m2m_aio_3a0_v19_5_4
* m2m_aio_3a0_v19_7_7

You can insert the microSD card into your PC and copy these files to it as a way
to get started.
