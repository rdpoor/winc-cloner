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

Since we're cloning the contents of the WINC1500, we neither need or want the
error checking that the WDRV_WINC_xxx() functions provide.  In fact, that error
checking prevents the driver from working with older WINC firmware.

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
#include "efuse.h"
#include "m2m_wifi.h"
#include "spi_flash.h"
#include "spi_flash_map.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

#define PLL_MAGIC_NUMBER 0x12345675
#define NUM_CHANNELS 14
#define NUM_FREQS 84

typedef struct {
  uint32_t u32PllInternal1;
  uint32_t u32PllInternal4;
  uint32_t WlanRx1;
  uint32_t WlanRx2;
  uint32_t WlanRx3;
  uint32_t WlanTx1;
  uint32_t WlanTx2;
  uint32_t WlanTx3;
} tstrChannelParm;

typedef enum {
  SECTOR_OKAY,
  SECTOR_ERROR,
  SECTOR_EQUAL,
  SECTOR_DIFFER,
  SECTOR_SKIPPED,
} sector_result_t;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Read one sector from WINC flash memory into dst.
 *
 * NOTE: addr must fall on a FLASH_SECTOR_SZ boundary.
 * NOTE: dst must be at least FLASH_SECTOR_SZ bytes big.
 */
static sector_result_t winc_sector_read(uint8_t *dst, uint32_t src_addr);

/**
 * @brief Write one sector to WINC flash memory from src.
 *
 * NOTE: addr must fall on a FLASH_SECTOR_SZ boundary.
 * NOTE: src must be at least FLASH_SECTOR_SZ bytes big.
 *
 * This function first reads a sector of data into a static buffer,
 * compares it against the src data.  If they differ, it erases the
 * sector and writes the src data to the WINC.  Otherwise, it leaves
 * the WINC flash untouched.
 */
static sector_result_t winc_sector_write(uint8_t *src, uint32_t dst_addr);

static bool cloner_aux(const char *filename,
                       SYS_FS_FILE_OPEN_ATTRIBUTES file_mode,
                       bool (*inner_loop)(SYS_FS_HANDLE file_handle,
                                          size_t n_bytes));

static bool extract_loop(SYS_FS_HANDLE file_handle, size_t n_bytes);
static bool update_loop(SYS_FS_HANDLE file_handle, size_t n_bytes);
static bool compare_loop(SYS_FS_HANDLE file_handle, size_t n_bytes);

static bool buffers_are_equal(uint8_t *buf_a, uint8_t *buf_b, size_t n_bytes);

static int32_t winc3400_pll_table_build(uint8_t *pBuffer, uint32_t freqOffset);

static bool open_winc(void);

static void dump_pll_data(uint8_t *buf, const char *msg);

// *****************************************************************************
// Private (static) storage

uint8_t s_xfer_buf[FLASH_SECTOR_SZ];  // transfer between WINC flash and file
uint8_t s_xfer_buf2[FLASH_SECTOR_SZ]; // transfer between WINC flash and file

EFUSEProdStruct efuseStruct = {0};
uint8_t pllFlashSector[M2M_PLL_FLASH_SZ] = {0};

static bool s_winc_is_opened;

// *****************************************************************************
// Public code

void winc_cloner_init(void) {
  s_winc_is_opened = false;
}

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

bool winc_cloner_rebuild_pll(void) {

  if (!open_winc()) {
    SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nCould not open WINC");
    return false;
  }

  // fetch a copy of the PLL / GAIN tables
  if (winc_sector_read(s_xfer_buf, M2M_PLL_FLASH_OFFSET) != SECTOR_OKAY) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "Could not read existing PLL / GAIN sector from WINC\r\n");
    return false;
  }

  dump_pll_data(s_xfer_buf, "before");

  // Read XO offset
  if (read_efuse_struct(&efuseStruct, 0) != EFUSE_SUCCESS) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR, "Failed to read the efuse table\r\n");
    return false;
  }

  int32_t ret = 0;
  // Overwrite the PLL section of in-RAM sector with newly computed PLL data.
  ret = winc3400_pll_table_build(s_xfer_buf, efuseStruct.FreqOffset);
  if (ret <= 0) {
    SYS_DEBUG_PRINT(
        SYS_ERROR_ERROR, "Failed to construct PLL table, err=%d\r\n", ret);
    return false;
  } else {
    SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                    "Successfully constructed PLL table with size %d bytes\r\n",
                    ret);
  }

  dump_pll_data(s_xfer_buf, "after");

  // Write the PLL / DATA sector to the WINC
  sector_result_t res = winc_sector_write(s_xfer_buf, M2M_PLL_FLASH_OFFSET);
  if (res == SECTOR_ERROR) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "Failed to write PLL / DATA sector to the WINC\r\n");
    return false;

  } else if (res == SECTOR_EQUAL) {
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "PLL / DATA sector up to date\r\n");

  } else if (res == SECTOR_DIFFER) {
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "PLL / DATA sector updated\r\n");
  }

  return true;
}

// *****************************************************************************
// Private (static) code

static sector_result_t winc_sector_read(uint8_t *dst, uint32_t src_addr) {
  if ((src_addr % FLASH_SECTOR_SZ) != 0) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "\nAddress 0x%lx not aligned with FLASH_SECTOR_SZ",
                    src_addr);
    return SECTOR_ERROR;
  }
  uint8_t ret = spi_flash_read(dst, src_addr, FLASH_SECTOR_SZ);
  if (ret != M2M_SUCCESS) {
    // WINC read failed.
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "\nFailed to read %ld WINC bytes at 0x%lx",
                    FLASH_SECTOR_SZ,
                    src_addr);
    return SECTOR_ERROR;
  }
  return SECTOR_OKAY;
}

static sector_result_t winc_sector_write(uint8_t *src, uint32_t dst_addr) {
  static uint8_t buf2[FLASH_SECTOR_SZ];

  if ((dst_addr % FLASH_SECTOR_SZ) != 0) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "\nAddress 0x%lx not aligned with FLASH_SECTOR_SZ",
                    dst_addr);
    return SECTOR_ERROR;
  }

  uint8_t ret = spi_flash_read(buf2, dst_addr, FLASH_SECTOR_SZ);
  if (ret != M2M_SUCCESS) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "\nFailed to read %ld WINC bytes at 0x%lx",
                    FLASH_SECTOR_SZ,
                    dst_addr);
    return SECTOR_ERROR;
  }

  if (buffers_are_equal(src, buf2, FLASH_SECTOR_SZ)) {
    // buffers are equal: return immediately
    return SECTOR_EQUAL;
  }

  // buffer differ: erase the sector and write from src
  if (spi_flash_erase(dst_addr, FLASH_SECTOR_SZ) != M2M_SUCCESS) {
    // winc erase failed
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "\nFailed to erase %ld WINC bytes at 0x%lx",
                    FLASH_SECTOR_SZ,
                    dst_addr);
    return SECTOR_ERROR;
  }

  // Sector has been erased.  Now write the data.
  if (spi_flash_write(src, dst_addr, FLASH_SECTOR_SZ) != M2M_SUCCESS) {
    // winc write failed
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "\nFailed to write %ld WINC bytes at 0x%lx",
                    FLASH_SECTOR_SZ,
                    dst_addr);
    return SECTOR_ERROR;
  }
  return SECTOR_DIFFER;
}

static bool cloner_aux(const char *filename,
                       SYS_FS_FILE_OPEN_ATTRIBUTES file_mode,
                       bool (*inner_loop)(SYS_FS_HANDLE file_handle,
                                          size_t n_bytes)) {
  SYS_FS_HANDLE file_handle;
  size_t n_bytes;
  uint8_t ret;

  if (!open_winc()) {
    SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nCould not open WINC");
    return false;
  }

  n_bytes = spi_flash_get_size() << 17; // convert megabits to bytes

  file_handle = SYS_FS_FileOpen(filename, file_mode);
  if (file_handle == SYS_FS_HANDLE_INVALID) {
    // Could not open file
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR, "\nCould not open file %s", filename);
    return false;
  }
  SYS_CONSOLE_MESSAGE("\n");
  ret = inner_loop(file_handle, n_bytes);
  SYS_FS_FileClose(file_handle); // assure that the file is closed

  return ret;
}

static bool extract_loop(SYS_FS_HANDLE file_handle, size_t n_bytes) {
  uint32_t src_addr = 0;

  while (n_bytes > 0) {
    size_t to_xfer = n_bytes;
    if (to_xfer > FLASH_SECTOR_SZ) {
      to_xfer = FLASH_SECTOR_SZ;
    }
    if (winc_sector_read(s_xfer_buf, src_addr) != SECTOR_OKAY) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld bytes at 0x%ld from WINC",
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
  sector_result_t res;

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

    if ((dst_addr >= M2M_PLL_FLASH_OFFSET) &&
        (dst_addr < M2M_PLL_FLASH_OFFSET + M2M_CONFIG_SECT_TOTAL_SZ)) {
      // do not overwrite PLL and GAIN settings: see spi_flash_map.h
      res = SECTOR_SKIPPED;
    } else {
      res = winc_sector_write(s_xfer_buf, dst_addr);
    }

    if (res == SECTOR_ERROR) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to write %ld bytes at address 0x%ld to WINC",
                      to_xfer,
                      dst_addr);
      return false;

    } else if (res == SECTOR_EQUAL) {
      SYS_CONSOLE_MESSAGE("=");

    } else if (res == SECTOR_DIFFER) {
      SYS_CONSOLE_MESSAGE("!");

    } else if (res == SECTOR_SKIPPED) {
      SYS_CONSOLE_MESSAGE("x");
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
    if (winc_sector_read(s_xfer_buf2, dst_addr) != SECTOR_OKAY) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld bytes at 0x%lx from WINC",
                      to_xfer,
                      dst_addr);
      return false;
    }
    if (buffers_are_equal(s_xfer_buf, s_xfer_buf2, to_xfer)) {
      // buffers are identical
      SYS_CONSOLE_MESSAGE("=");
    } else {
      // buffers differ
      SYS_CONSOLE_MESSAGE("!");
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

static int32_t winc3400_pll_table_build(uint8_t *pBuffer, uint32_t freqOffset) {
  uint32_t val32;
  uint32_t magic[2];
  tstrChannelParm strChnParm[NUM_CHANNELS];
  uint32_t
      strFreqParam[NUM_FREQS + 1]; /* 1 extra (1920.0) for cpll compensate */
  uint8_t ch, freq;
  int32_t i32xo_offset;
  double xo_offset;
  double xo_to_VCO;
  double lo;

  if (NULL == pBuffer) {
    SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                    "error: '%s' invalid parameters supplied\r\n",
                    __FUNCTION__);
    return -1;
  }

  i32xo_offset = (freqOffset > (1 << 14)) ? freqOffset - (1 << 15) : freqOffset;
  xo_offset = ((double)i32xo_offset) / (1 << 6);
  xo_to_VCO = 2 * 26.0 * (1 + (xo_offset / 1000000.0));

  SYS_CONSOLE_PRINT("Creating WiFi channel lookup table for PLL with "
                    "xo_offset = %3.4lf\r\n",
                    xo_offset);

  for (ch = 0, lo = 4824.0; ch < NUM_CHANNELS; ch++, lo += 10) {
    uint32_t n2, f, m, g;
    double lo_actual;
    double n1, dec, inv;
    double gMoG;

    if (ch == 13)
      lo = 4968.0;

    n2 = (uint32_t)(lo / xo_to_VCO);
    f = (uint32_t)(((lo / xo_to_VCO) - n2) * (1 << 19) + 0.5);

    lo_actual = (double)xo_to_VCO * (double)(n2 + ((double)f / (1 << 19)));

    val32 = ((n2 & 0x1fful) << 19) | ((f & 0x7fffful) << 0);
    val32 |= (1ul << 31);

    strChnParm[ch].u32PllInternal1 = val32;

    m = (uint32_t)(lo_actual / 80.0);
    g = (uint32_t)((lo_actual / 80.0 - m) * (1 << 19));
    gMoG = (double)(m + ((double)g / (1 << 19)));

    val32 = ((m & 0x1fful) << 19) | ((g & 0x7fffful) << 0);
    val32 &= ~(1ul << 28); /* Dither must be disbled */

    strChnParm[ch].u32PllInternal4 = val32;

    n1 = (uint32_t)trunc(((60.0 / gMoG) * (1ul << 22)));
    dec = (uint32_t)round((((60.0 / gMoG) * (1ul << 22)) - n1) * (1ul << 31));
    inv = (uint32_t)trunc(((1ul << 22) / (n1 / (1ul << 11))) + 0.5);

    strChnParm[ch].WlanRx1 = (uint32_t)n1;
    strChnParm[ch].WlanRx3 = (uint32_t)dec;
    strChnParm[ch].WlanRx2 = (uint32_t)inv;

    n1 = (uint32_t)trunc(((gMoG / 60.0) * (1ul << 22)));
    dec = (uint32_t)round((((gMoG / 60.0) * (1ul << 22)) - n1) * (1ul << 31));
    inv = (uint32_t)trunc(((1ul << 22) / (n1 / (1ul << 11))) + 0.5);

    strChnParm[ch].WlanTx1 = (uint32_t)n1;
    strChnParm[ch].WlanTx3 = (uint32_t)dec;
    strChnParm[ch].WlanTx2 = (uint32_t)inv;
  }

  SYS_CONSOLE_PRINT(
      "Creating frequency lookup table for PLL with xo_offset = %3.4f.\r\n",
      xo_offset);

  for (freq = 0, lo = 3840.0; freq < NUM_FREQS + 1; freq++, lo += 2) {
    uint32_t n2, f;

    if (freq == 1)
      lo = 4802.0;

    n2 = (uint32_t)(lo / xo_to_VCO);
    f = (uint32_t)(((lo / xo_to_VCO) - n2) * (1 << 19) + 0.5);

    strFreqParam[freq] = ((n2 & 0x1fful) << 19) | ((f & 0x7fffful) << 0);
  }

  magic[0] = PLL_MAGIC_NUMBER;
  magic[1] = freqOffset;

  memcpy(pBuffer, &magic, sizeof(magic));
  pBuffer += sizeof(magic);

  memcpy(pBuffer, &strChnParm, sizeof(strChnParm));
  pBuffer += sizeof(strChnParm);

  memcpy(pBuffer, &strFreqParam, sizeof(strFreqParam));
  return sizeof(magic) + sizeof(strChnParm) + sizeof(strFreqParam);
}

static bool open_winc(void) {
  if (!s_winc_is_opened) {
    if (m2m_wifi_download_mode() != M2M_SUCCESS) {
      // Could not enter WINC download mode.
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nCould not access WINC");
      s_winc_is_opened = true;
    } else {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nWINC opened");
      s_winc_is_opened = true;
    }
  }
  return s_winc_is_opened;
}

// #define PLL_TABLE_LEN 796
#define PLL_TABLE_LEN 64

static void dump_pll_data(uint8_t *buf, const char *msg) {
  SYS_CONSOLE_PRINT("\n%s", msg);

  for (int i=0; i<PLL_TABLE_LEN; i++) {
    if ((i % 32) == 0) {
      SYS_CONSOLE_MESSAGE("\n");
    }
    SYS_CONSOLE_PRINT(" %02x", buf[i]);
  }
  SYS_CONSOLE_MESSAGE("\n");
}

// *****************************************************************************
// End of file
