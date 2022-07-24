/**
 * @file line_reader.c
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

#include "line_reader.h"

#include "definitions.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define MAX_CHARS 100

#define STATES(M)                                                              \
  M(LINE_READER_STATE_INIT)                                                    \
  M(LINE_READER_STATE_START_READING)                                           \
  M(LINE_READER_STATE_AWAIT_LINE)                                              \
  M(LINE_READER_STATE_SUCCESS)                                                 \
  M(LINE_READER_STATE_ERROR)

#define EXPAND_STATE_IDS(_name) _name,
typedef enum { STATES(EXPAND_STATE_IDS) } line_reader_state_t;

typedef struct {
  line_reader_state_t state;
  uint8_t linebuf[MAX_CHARS];
  uint8_t n_read;
} line_reader_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the internal state.
 */
static void set_state(line_reader_state_t state);

/**
 * @brief Return the name of the given state.
 */
static const char *state_name(line_reader_state_t state);

// *****************************************************************************
// Private (static) storage

#define EXPAND_STATE_NAMES(_name) #_name,
static const char *s_state_names[] = {STATES(EXPAND_STATE_NAMES)};

#define N_STATES (sizeof(s_state_names) / sizeof(s_state_names[0]))

static line_reader_ctx_t s_line_reader_ctx;

// *****************************************************************************
// Public code

void line_reader_init(void) {
  s_line_reader_ctx.state = LINE_READER_STATE_INIT;
}

/**
 * @brief Step the demo task internal state.  Called frequently.
 */
void line_reader_step(void) {
  switch (s_line_reader_ctx.state) {

  case LINE_READER_STATE_INIT: {
    // wait here for a call to line_reader_start()
  } break;

  case LINE_READER_STATE_START_READING: {
    memset(s_line_reader_ctx.linebuf, 0, sizeof(s_line_reader_ctx.linebuf));
    s_line_reader_ctx.n_read = 0;
    set_state(LINE_READER_STATE_AWAIT_LINE);
  } break;

  case LINE_READER_STATE_AWAIT_LINE: {
    uint8_t *dst = &s_line_reader_ctx.linebuf[s_line_reader_ctx.n_read];
    size_t n_read = SYS_CONSOLE_Read(SYS_CONSOLE_DEFAULT_INSTANCE,
                                     dst,
                                     MAX_CHARS - s_line_reader_ctx.n_read);
    if (n_read < 0) {
      set_state(LINE_READER_STATE_ERROR);

    } else if (s_line_reader_ctx.n_read == MAX_CHARS) {
      // line buffer full without EOL seen.  Just terminate it and continue.
      s_line_reader_ctx.linebuf[MAX_CHARS-1] = '\0';
      set_state(LINE_READER_STATE_SUCCESS);

    } else {
      // Zero or more chars were read.  Was EOL seen (either \r or \n)?
      SYS_CONSOLE_MESSAGE((const char *)dst);  // echo
      for (int i=0; i<n_read; i++) {
        if (dst[i] == '\r' || dst[i] == '\n') {
          dst[i] = '\0';  // null terminate
          set_state(LINE_READER_STATE_SUCCESS);
          break;
        } else if (dst[i] == '\e') {
          // User typed escape -- abort this command.
          dst[i] = '\0';
          set_state(LINE_READER_STATE_ERROR);
          break;
        }
      }
      s_line_reader_ctx.n_read += n_read;
      // no line terminator seen - remain in this state.
    }
  } break;

  case LINE_READER_STATE_SUCCESS: {
    // here once a line successfully read in.  Fetch via line_reader_get_line().
  } break;

  case LINE_READER_STATE_ERROR: {
    // here on error state
  } break;
  } // switch
}

void line_reader_start(void) {
  s_line_reader_ctx.state = LINE_READER_STATE_START_READING;
}

const char *line_reader_get_line(void) {
  return (const char *)s_line_reader_ctx.linebuf;
}

bool line_reader_succeeded(void) {
  return s_line_reader_ctx.state == LINE_READER_STATE_SUCCESS;
}

bool line_reader_has_error(void) {
  return s_line_reader_ctx.state == LINE_READER_STATE_ERROR;
}

// *****************************************************************************
// Private (static) code

static void set_state(line_reader_state_t state) {
  if (s_line_reader_ctx.state != state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "%s => %s",
                    state_name(s_line_reader_ctx.state),
                    state_name(state));
    s_line_reader_ctx.state = state;
  }
}

static const char *state_name(line_reader_state_t state) {
  SYS_ASSERT(state < N_STATES, "line_reader_state_t out of bounds");
  return s_state_names[state];
}

// *****************************************************************************
// End of file
