/**
 * @file template_task.h
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
 * @brief template_task is a canonical template for any state-driven task.
 */

#ifndef _TEMPLATE_TASK_H_
#define _TEMPLATE_TASK_H_

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
typedef void (*template_task_callback_fn)(uintptr_t arg);

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the template_task.  Called once at startup.
 */
void template_task_init(void);

/**
 * @brief Step the template_task internal state.  Called frequently.
 */
void template_task_step(void);

/**
 * @brief Set a callback to be triggered when the template_task completes.
 */
void template_task_set_callback(template_task_callback_fn callback_fn,
                                uintptr_t callback_arg);

/**
 * @brief Return true if the template_task is idle.
 */
bool template_task_is_idle(void);

/**
 * @brief Return true if the template_task has encountered an error.
 */
bool template_task_has_error(void);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _TEMPLATE_TASK_H_ */
