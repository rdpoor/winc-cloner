/**
 * @file template_task.c
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

// *****************************************************************************
// Includes

#include "template_task.h"

#include "assert.h"
#include "dt3_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define STATES(M)                                                              \
  M(TEMPLATE_TASK_STATE_IDLE)                                                  \
  M(TEMPLATE_TASK_STATE_ERROR)

#define EXPAND_STATE_IDS(_name) _name,
typedef enum { STATES(EXPAND_STATE_IDS) } template_task_state_t;

typedef struct {
  template_task_state_t state;
  template_task_callback_fn callback_fn;
  uintptr_t callback_arg;
} template_task_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the internal state.
 */
static void set_state(template_task_state_t state);

/**
 * @brief Return the name of the given state.
 */
static const char *state_name(template_task_state_t state);

/**
 * @brief If a callback function has been set, invoke it with the callback arg.
 */
static void trigger_callback(void);

// *****************************************************************************
// Private (static) storage

#define EXPAND_STATE_NAMES(_name) #_name,
static const char *s_state_names[] = {STATES(EXPAND_STATE_NAMES)};

#define N_STATES (sizeof(s_state_names) / sizeof(s_state_names[0]))

static template_task_ctx_t s_template_task_ctx;

// *****************************************************************************
// Public code

void template_task_init(void) {
  s_template_task_ctx.state = TEMPLATE_TASK_STATE_IDLE;
}

/**
 * @brief Step the demo task internal state.  Called frequently.
 */
void template_task_step(void) {
  switch (s_template_task_ctx.state) {
  case TEMPLATE_TASK_STATE_IDLE: {
    // here on idle state.
  } break;
  case TEMPLATE_TASK_STATE_ERROR: {
    // here on error state
  } break;
  } // switch
}

void template_task_set_callback(template_task_callback_fn callback_fn,
                                uintptr_t callback_arg) {
  s_template_task_ctx.callback_fn = callback_fn;
  s_template_task_ctx.callback_arg = callback_arg;
}

bool template_task_is_idle(void) {
  return s_template_task_ctx.state == TEMPLATE_TASK_STATE_IDLE;
}

bool template_task_has_error(void) {
  return s_template_task_ctx.state == TEMPLATE_TASK_STATE_ERROR;
}

// *****************************************************************************
// Private (static) code

static void set_state(template_task_state_t state) {
  if (s_template_task_ctx.state != state) {
    DT3_LOG_DEBUG(
        "%s => %s", state_name(s_template_task_ctx.state), state_name(state));
    s_template_task_ctx.state = state;
  }
}

static const char *state_name(template_task_state_t state) {
  ASSERT(state < N_STATES);
  return s_state_names[state];
}

static void trigger_callback(void) {
  if (s_template_task_ctx.callback_fn) {
    s_template_task_ctx.callback_fn(s_template_task_ctx.callback_arg);
  }
}

// *****************************************************************************
// End of file
