/**
 * @file dir_reader.h
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
 * @brief dir_reader lists the available .img files in the root directory.
 */

#ifndef _DIR_READER_H_
#define _DIR_READER_H_

// *****************************************************************************
// Includes

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

/**
 * @brief Signature for the callback function.
 */
typedef void (*dir_reader_callback_fn)(uintptr_t arg);

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the dir_reader.  Called once at startup.
 */
void dir_reader_init(void);

/**
 * @brief Step the dir_reader internal state.  Called frequently.
 */
void dir_reader_step(void);

/**
 * @brief Set a callback to be triggered when the dir_reader completes.
 */
void dir_reader_set_callback(dir_reader_callback_fn callback_fn,
                             uintptr_t callback_arg);

/**
 * @brief Read the rood directory to discover the .img files.
 *
 * Note: this is asynchronous.  The results are avaiable after
 * dir_reader_is_complete() returns true.
 */
void dir_reader_read_directory(void);

/**
 * @brief Return the number of .img files found in the root directory.
 *
 * Note: valid only after dir_reader_is_complete() returns true.
 */
uint8_t dir_reader_filename_count(void);

/**
 * @brief Return the idx'th image filename, or NULL if idx is out of bounds.
 *
 * Note: valid only after dir_reader_is_complete() returns true.
 */
const char *dir_reader_filename_ref(uint8_t idx);

/**
 * @brief Return true if the dir_reader is idle.
 */
bool dir_reader_is_idle(void);

/**
 * @brief Return true if the dir_reader completed successfully
 */
bool dir_reader_is_complete(void);

/**
 * @brief Return true if the dir_reader has encountered an error.
 */
bool dir_reader_has_error(void);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _DIR_READER_H_ */
