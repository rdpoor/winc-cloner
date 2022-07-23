/**
 * @file winc_cloner.c
 *
 * MIT License
 *
 * Copyright (c) 2022 R. D. Poor (https://github.com/rdpoor)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
Implementation notes:

This module uses high-level SYS_FS_xxx() calls for all file operations.  But for
WINC operations, we intentionally use the low level m2m_wifi_() and spi_flash_()
methods as described in Section 13 of:

  http://ww1.microchip.com/downloads/en/devicedoc/atmel-42420-winc1500-software-design-guide_userguide.pdf

Including:
 sint32 m2m_wifi_download_mode();
 uint32 spi_flash_get_size();
 sint8 spi_flash_read(uint8 *pu8Buf, uint32 u32offset, uint32 u32Sz)
 sint8 spi_flash_erase(uint32 u32Offset, uint32 u32Sz)
 sint8 spi_flash_write(uint8* pu8Buf, uint32 u32Offset, uint32 u32Sz)

Since we're cloning the contents of the WINC1500, we don't need or want the
error checking that the WDRV_WINC_xxx() functions provide.  In fact, they won't
work with older versions of the WINC firmware.

Code example from the user guide:

#include "spi_flash.h"

#define DATA_TO_REPLACE "THIS IS A NEW SECTOR IN FLASH"

int main() {
 uint8au8FlashContent[FLASH_SECTOR_SZ] = {0};
 uint32u32FlashTotalSize = 0, u32FlashOffset = 0;

 // Platform specific initializations.
 ret = m2m_wifi_download_mode();
 if (M2M_SUCCESS != ret) {
   printf("Unable to enter download mode\r\n");
 } else {
   u32FlashTotalSize = spi_flash_get_size();
 }
 while ((u32FlashTotalSize > u32FlashOffset) && (M2M_SUCCESS == ret)) {
   ret = spi_flash_read(au8FlashContent, u32FlashOffset, FLASH_SECTOR_SZ);
   if (M2M_SUCCESS != ret) {
     printf("Unable to read SPI sector\r\n");
     break;
   }
   memcpy(au8FlashContent, DATA_TO_REPLACE, strlen(DATA_TO_REPLACE));
   ret = spi_flash_erase(u32FlashOffset, FLASH_SECTOR_SZ);
   if (M2M_SUCCESS != ret) {
     printf("Unable to erase SPI sector\r\n");
     break;
   }
   ret = spi_flash_write(au8FlashContent, u32FlashOffset, FLASH_SECTOR_SZ);
   if (M2M_SUCCESS != ret) {
     printf("Unable to write SPI sector\r\n");
     break;
   }
   u32FlashOffset += FLASH_SECTOR_SZ;
 }
 if (M2M_SUCCESS == ret) {
   printf("Successful operations\r\n");
 } else {
   printf("Failed operations\r\n");
 }
 while (1)
   ;
 return M2M_SUCCESS;
}

*/

// *****************************************************************************
// Includes

#include "winc_cloner.h"

#include "definitions.h"
#include "m2m_wifi.h"
#include "spi_flash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static, forward) declarations

static bool cloner_aux(const char *filename,
                       SYS_FS_FILE_OPEN_ATTRIBUTES file_mode,
                       bool (*inner_loop)(SYS_FS_HANDLE file_handle,
                                          size_t n_bytes));

static bool extract_loop(SYS_FS_HANDLE file_handle, size_t n_bytes);
static bool update_loop(SYS_FS_HANDLE file_handle, size_t n_bytes);
static bool compare_loop(SYS_FS_HANDLE file_handle, size_t n_bytes);

static bool buffers_are_equal(uint8_t *buf_a, uint8_t *buf_b, size_t n_bytes);

// *****************************************************************************
// Private (static) storage

uint8_t s_xfer_buf[FLASH_SECTOR_SZ];  // transfer between WINC flash and file
uint8_t s_xfer_buf2[FLASH_SECTOR_SZ]; // transfer between WINC flash and file

// *****************************************************************************
// Public code

void winc_cloner_init(void) {}

bool winc_cloner_extract(const char *filename) {
  bool ret = cloner_aux(filename, SYS_FS_FILE_OPEN_WRITE, extract_loop);
  if (ret) {
    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                    "\nSuccessfully extracted WINC contents into %s",
                    filename);
  }
  return ret;
}

bool winc_cloner_update(const char *filename) {
  bool ret = cloner_aux(filename, SYS_FS_FILE_OPEN_READ, update_loop);
  if (ret) {
    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                    "\nSuccessfully updated WINC contents from %s",
                    filename);
  }
  return ret;
}

bool winc_cloner_compare(const char *filename) {
  bool ret = cloner_aux(filename, SYS_FS_FILE_OPEN_READ, compare_loop);
  if (ret) {
    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                    "\nSuccessfully compared WINC contents to %s",
                    filename);
  }
  return ret;
}

// *****************************************************************************
// Private (static) code

static bool cloner_aux(const char *filename,
                       SYS_FS_FILE_OPEN_ATTRIBUTES file_mode,
                       bool (*inner_loop)(SYS_FS_HANDLE file_handle,
                                          size_t n_bytes)) {
  SYS_FS_HANDLE file_handle;
  size_t n_bytes;
  uint8_t ret;

  if (m2m_wifi_download_mode() != M2M_SUCCESS) {
    // Could not enter WINC download mode.
    SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nCould not access WINC");
    return false;
  }
  n_bytes = spi_flash_get_size() << 17; // convert megabits to bytes

  file_handle = SYS_FS_FileOpen(filename, file_mode);
  if (file_handle == SYS_FS_HANDLE_INVALID) {
    // Could not open file
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR, "\nCould not open file %s", filename);
    return false;
  }
  ret = inner_loop(file_handle, n_bytes);
  SYS_FS_FileClose(file_handle); // assure that the file is closed

  return ret;
}

static bool extract_loop(SYS_FS_HANDLE file_handle, size_t n_bytes) {
  uint32_t src_addr = 0;
  uint8_t ret;

  while (n_bytes > 0) {
    size_t to_xfer = n_bytes;
    if (to_xfer > FLASH_SECTOR_SZ) {
      to_xfer = FLASH_SECTOR_SZ;
    }
    ret = spi_flash_read(s_xfer_buf, src_addr, to_xfer);
    if (ret != M2M_SUCCESS) {
      // WINC read failed.
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld WINC bytes at 0x%lx",
                      to_xfer,
                      src_addr);
      return false;
    }
    if (SYS_FS_FileWrite(file_handle, s_xfer_buf, to_xfer) < 0) {
      // file write failed
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nFailed to write %ld bytes to file", to_xfer);
      return false;
    }
    n_bytes -= to_xfer;
    src_addr += to_xfer;
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, ".");
  }
  // success
  return true;
}

static bool update_loop(SYS_FS_HANDLE file_handle, size_t n_bytes) {
  uint32_t dst_addr = 0;
  uint8_t ret;

  while (n_bytes > 0) {
    size_t to_xfer = n_bytes;
    if (to_xfer > FLASH_SECTOR_SZ) {
      to_xfer = FLASH_SECTOR_SZ;
    }
    // Read a sector of data from the file and from the WINC.  If they differ,
    // erase the sector and write the file data to the WINC.
    if (SYS_FS_FileRead(file_handle, s_xfer_buf, to_xfer) < 0) {
      // file read failed.
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nFailed to read %ld bytes from file", to_xfer);
      return false;
    }
    ret = spi_flash_read(s_xfer_buf2, dst_addr, to_xfer);
    if (ret != M2M_SUCCESS) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld WINC bytes at 0x%lx",
                      to_xfer,
                      dst_addr);
      return false;
    }
    if (buffers_are_equal(s_xfer_buf, s_xfer_buf2, to_xfer)) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, ".");
    } else {
      // buffer differ: erase the sector and write from file
      if (spi_flash_erase(dst_addr, to_xfer) != M2M_SUCCESS) {
        // winc erase failed
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                        "\nFailed to erase %ld WINC bytes at 0x%lx",
                        to_xfer,
                        dst_addr);
        return false;
      }
      if (spi_flash_write(s_xfer_buf, dst_addr, to_xfer) != M2M_SUCCESS) {
        // winc write failed
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                        "\nFailed to write %ld WINC bytes at 0x%lx",
                        to_xfer,
                        dst_addr);
        return false;
      }
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "!");
    }
    // advance to next sector
    n_bytes -= to_xfer;
    dst_addr += to_xfer;
  }
  // success
  return true;
}

static bool compare_loop(SYS_FS_HANDLE file_handle, size_t n_bytes) {
  uint32_t dst_addr = 0;
  uint8_t ret;

  while (n_bytes > 0) {
    size_t to_xfer = n_bytes;
    if (to_xfer > FLASH_SECTOR_SZ) {
      to_xfer = FLASH_SECTOR_SZ;
    }
    // Read a sector of data from the file and from the WINC and compare them.
    if (SYS_FS_FileRead(file_handle, s_xfer_buf, to_xfer) < 0) {
      // file read failed.
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nFailed to read %ld bytes from file", to_xfer);
      return false;
    }
    ret = spi_flash_read(s_xfer_buf2, dst_addr, to_xfer);
    if (ret != M2M_SUCCESS) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld WINC bytes at 0x%lx",
                      to_xfer,
                      dst_addr);
      return false;
    }
    if (buffers_are_equal(s_xfer_buf, s_xfer_buf2, to_xfer)) {
      // buffers are identical
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, ".");
    } else {
      // buffers differ
      SYS_CONSOLE_PRINT("\nWINC and file differ at sector 0x%lx", dst_addr);
    }
    // advance to next sector
    n_bytes -= to_xfer;
    dst_addr += to_xfer;
  }
  // success
  return true;
}

static bool buffers_are_equal(uint8_t *buf_a, uint8_t *buf_b, size_t n_bytes) {
  for (size_t i = 0; i < n_bytes; i++) {
    if (buf_a[i] != buf_b[i]) {
      return false;
    }
  }
  return true;
}

// *****************************************************************************
// End of file
