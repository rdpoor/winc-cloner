/**
 * @file winc_cloner.h
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
 */

/**
 * @brief winc_cloner extracts, updates, or compares a WINC1500 flash image.
 */

#ifndef _WINC_CLONER_H_
#define _WINC_CLONER_H_

// *****************************************************************************
// Includes

#include <stdbool.h>

// *****************************************************************************
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the winc_cloner.  Called once at startup.
 */
void winc_cloner_init(void);

/**
 * @brief Extract the entire contents of the WINC firmware image into a file.
 *
 * @return true on success
 */
bool winc_cloner_extract(const char *filename);

/**
 * @brief Update the contents of the WINC firmware image from a file.
 *
 * Note: winc_cloner_update() does not touch the PLL and GAIN tables.
 *
 * @return true on success
 */
bool winc_cloner_update(const char *filename);

/**
 * @brief Compare the entire contents of the WINC firmware image with a file.
 *
 * @return true on if the WINC firmware is identical to the file contents.
 */
bool winc_cloner_compare(const char *filename);

/**
 * @brief Rebuild the PLL tables.  Required if gain table have changed, or if
 * the PLL tables were clobbered by winc-cloner v 0.0.3 or earlier.
 *
 * @return true on success
 */
bool winc_cloner_rebuild_pll(void);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _WINC_CLONER_H_ */
