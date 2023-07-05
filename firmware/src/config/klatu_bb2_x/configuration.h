/*******************************************************************************
  System Configuration Header

  File Name:
    configuration.h

  Summary:
    Build-time configuration header for the system defined by this project.

  Description:
    An MPLAB Project may have multiple configurations.  This file defines the
    build-time options for a single configuration.

  Remarks:
    This configuration header must not define any prototypes or data
    definitions (or include any files that do).  It only provides macro
    definitions for build-time configuration options

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
/*  This section Includes other configuration headers necessary to completely
    define this configuration.
*/

#include "user.h"
#include "device.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: System Configuration
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: System Service Configuration
// *****************************************************************************
// *****************************************************************************

#define SYS_DEBUG_ENABLE
#define SYS_DEBUG_GLOBAL_ERROR_LEVEL       SYS_ERROR_INFO
#define SYS_DEBUG_BUFFER_DMA_READY
#define SYS_DEBUG_USE_CONSOLE


/* TIME System Service Configuration Options */
#define SYS_TIME_INDEX_0                            (0)
#define SYS_TIME_MAX_TIMERS                         (5)
#define SYS_TIME_HW_COUNTER_WIDTH                   (16)
#define SYS_TIME_TICK_FREQ_IN_HZ                    (1000)


/* File System Service Configuration */

#define SYS_FS_MEDIA_NUMBER               1
#define SYS_FS_VOLUME_NUMBER              1

#define SYS_FS_AUTOMOUNT_ENABLE           true
#define SYS_FS_CLIENT_NUMBER              1
#define SYS_FS_MAX_FILES                  1
#define SYS_FS_MAX_FILE_SYSTEM_TYPE       1
#define SYS_FS_MEDIA_MAX_BLOCK_SIZE       512
#define SYS_FS_MEDIA_MANAGER_BUFFER_SIZE  2048
#define SYS_FS_USE_LFN                    1
#define SYS_FS_FILE_NAME_LEN              255
#define SYS_FS_CWD_STRING_LEN             1024


#define SYS_FS_FAT_VERSION                "v0.14b"
#define SYS_FS_FAT_READONLY               false
#define SYS_FS_FAT_CODE_PAGE              437
#define SYS_FS_FAT_MAX_SS                 SYS_FS_MEDIA_MAX_BLOCK_SIZE




#define SYS_FS_MEDIA_TYPE_IDX0 				SYS_FS_MEDIA_TYPE_SD_CARD
#define SYS_FS_TYPE_IDX0 					FAT
					
#define SYS_FS_MEDIA_IDX0_MOUNT_NAME_VOLUME_IDX0 			"/mnt/mydrive"
#define SYS_FS_MEDIA_IDX0_DEVICE_NAME_VOLUME_IDX0			"/dev/mmcblka1"
								

#define SYS_CONSOLE_DEVICE_MAX_INSTANCES   			1
#define SYS_CONSOLE_UART_MAX_INSTANCES 	   			1
#define SYS_CONSOLE_USB_CDC_MAX_INSTANCES 	   		0
#define SYS_CONSOLE_PRINT_BUFFER_SIZE        		200


#define SYS_CONSOLE_INDEX_0                       0






// *****************************************************************************
// *****************************************************************************
// Section: Driver Configuration
// *****************************************************************************
// *****************************************************************************
/* SDMMC Driver Global Configuration Options */
#define DRV_SDMMC_INSTANCES_NUMBER                       1

/*** WiFi WINC Driver Configuration ***/
#define WDRV_WINC_EIC_SOURCE
#define WDRV_WINC_NETWORK_MODE_SOCKET
#define WDRV_WINC_DEVICE_WINC1500
#define WDRV_WINC_DEVICE_SPLIT_INIT
#define WDRV_WINC_DEVICE_ENTERPRISE_CONNECT
#define WDRV_WINC_DEVICE_EXT_CONNECT_PARAMS
#define WDRV_WINC_DEVICE_BSS_ROAMING
#define WDRV_WINC_DEVICE_FLEXIBLE_FLASH_MAP
#define WDRV_WINC_DEVICE_DYNAMIC_BYPASS_MODE
#define WDRV_WINC_DEVICE_WPA_SOFT_AP
#define WDRV_WINC_DEVICE_CONF_NTP_SERVER
#define WDRV_WINC_DEVICE_HOST_FILE_DOWNLOAD
#define WDRV_WINC_DEVICE_SOFT_AP_EXT
#define WDRV_WINC_DEVICE_MULTI_GAIN_TABLE
#define WDRV_WINC_DEVICE_URL_TYPE           unsigned char
#define WDRV_WINC_DEVICE_SCAN_STOP_ON_FIRST
#define WDRV_WINC_DEVICE_DEPRECATE_WEP
#define WDRV_WINC_DEVICE_OTA_SSL_OPTIONS
#define WDRV_WINC_DEVICE_OTA_STATUS_EXTENDED
#define WDRV_WINC_DEVICE_SCAN_SSID_LIST
#define WDRV_WINC_DEVICE_USE_SYS_DEBUG

/* SPI Driver Instance 0 Configuration Options */
#define DRV_SPI_INDEX_0                       0
#define DRV_SPI_CLIENTS_NUMBER_IDX0           1
#define DRV_SPI_DMA_MODE
#define DRV_SPI_XMIT_DMA_CH_IDX0              SYS_DMA_CHANNEL_1
#define DRV_SPI_RCV_DMA_CH_IDX0               SYS_DMA_CHANNEL_0
#define DRV_SPI_QUEUE_SIZE_IDX0               4


/*** SDMMC Driver Instance 0 Configuration ***/
#define DRV_SDMMC_INDEX_0                                0
#define DRV_SDMMC_CLIENTS_NUMBER_IDX0                    1
#define DRV_SDMMC_QUEUE_SIZE_IDX0                        2
#define DRV_SDMMC_PROTOCOL_SUPPORT_IDX0                  DRV_SDMMC_PROTOCOL_SD
#define DRV_SDMMC_CONFIG_SPEED_MODE_IDX0                 DRV_SDMMC_SPEED_MODE_DEFAULT
#define DRV_SDMMC_CONFIG_BUS_WIDTH_IDX0                  DRV_SDMMC_BUS_WIDTH_4_BIT
#define DRV_SDMMC_CARD_DETECTION_METHOD_IDX0             DRV_SDMMC_CD_METHOD_USE_SDCD



/* SPI Driver Common Configuration Options */
#define DRV_SPI_INSTANCES_NUMBER              1



// *****************************************************************************
// *****************************************************************************
// Section: Middleware & Other Library Configuration
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Application Configuration
// *****************************************************************************
// *****************************************************************************


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif // CONFIGURATION_H
/*******************************************************************************
 End of File
*/
