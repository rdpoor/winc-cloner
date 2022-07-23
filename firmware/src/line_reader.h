/**
 * @file line_reader.h
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
 * @brief line_reader reads a line of characters on the console.
 */

#ifndef _LINE_READER_H_
#define _LINE_READER_H_

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
typedef void (*line_reader_callback_fn)(uintptr_t arg);

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the line_reader.  Called once at startup.
 */
void line_reader_init(void);

/**
 * @brief Step the line_reader internal state.  Called frequently.
 */
void line_reader_step(void);

/**
 * @brief Start reading a line from the console.
 */
void line_reader_start(void);

/**
 * @brief Return the most recerntly line read from the console.
 */
const char *line_reader_get_line(void);

/**
 * @brief Return true if the line reader completed without error.
 */
bool line_reader_succeeded(void);

/**
 * @brief Return true if the line_reader has encountered an error.
 */
bool line_reader_has_error(void);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _LINE_READER_H_ */
