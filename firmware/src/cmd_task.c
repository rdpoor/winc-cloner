/**
 * @file cmd_task.c
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

#include "cmd_task.h"

#include "app.h"
#include "definitions.h"
#include "dir_reader.h"
#include "line_reader.h"
#include "winc_cloner.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define STATES(M)                                                              \
  M(CMD_TASK_STATE_INIT)                                                       \
  M(CMD_TASK_STATE_PRINTING_HELP)                                              \
  M(CMD_TASK_STATE_READING_DIRECTORY)                                          \
  M(CMD_TASK_STATE_LISTING_DIRECTORY)                                          \
  M(CMD_TASK_STATE_AWAIT_COMMAND)                                              \
  M(CMD_TASK_STATE_START_EXTRACTING)                                           \
  M(CMD_TASK_STATE_START_UPDATING)                                             \
  M(CMD_TASK_STATE_START_COMPARING)                                            \
  M(CMD_TASK_STATE_START_REBUILDING)                                           \
  M(CMD_TASK_STATE_ERROR)

#define EXPAND_STATE_IDS(_name) _name,
typedef enum { STATES(EXPAND_STATE_IDS) } cmd_task_state_t;

typedef struct {
  cmd_task_state_t state;
} cmd_task_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the internal state.
 */
static void set_state(cmd_task_state_t state);

/**
 * @brief Return the name of the given state.
 */
static const char *state_name(cmd_task_state_t state);

static void flush_serial_input(void);

// *****************************************************************************
// Private (static) storage

#define EXPAND_STATE_NAMES(_name) #_name,
static const char *s_state_names[] = {STATES(EXPAND_STATE_NAMES)};

#define N_STATES (sizeof(s_state_names) / sizeof(s_state_names[0]))

static cmd_task_ctx_t s_cmd_task_ctx;

// *****************************************************************************
// Public code

void cmd_task_init(void) { s_cmd_task_ctx.state = CMD_TASK_STATE_INIT; }

/**
 * @brief Step the demo task internal state.  Called frequently.
 */
void cmd_task_step(void) {
  switch (s_cmd_task_ctx.state) {
  case CMD_TASK_STATE_INIT: {
    // here on idle state.
    set_state(CMD_TASK_STATE_PRINTING_HELP);
  } break;

  case CMD_TASK_STATE_PRINTING_HELP: {
    APP_PrintBanner();
    dir_reader_read_directory();
    set_state(CMD_TASK_STATE_READING_DIRECTORY);
  } break;

  case CMD_TASK_STATE_READING_DIRECTORY: {
    dir_reader_step();
    if (dir_reader_is_complete()) {
      set_state(CMD_TASK_STATE_LISTING_DIRECTORY);
    } else if (dir_reader_has_error()) {
      set_state(CMD_TASK_STATE_ERROR);
    } else {
      // remain in this state until dir_reader completes
    }
  } break;

  case CMD_TASK_STATE_LISTING_DIRECTORY: {
    // Here when dir_reader has completed successfully
    uint8_t count = dir_reader_filename_count();
    SYS_CONSOLE_PRINT("\nFound %d file%s", count, count == 1 ? "" : "s");
    for (uint8_t idx = 0; idx < count; idx++) {
      SYS_CONSOLE_PRINT("\n   %s", dir_reader_filename_ref(idx));
    }
    SYS_CONSOLE_MESSAGE("\nCommands:"
                        "\nh: print this help"
                        "\ne: extract WINC firmware to a file"
                        "\nu: update WINC firmware from a file"
                        "\nc: compare WINC firmware against a file"
                        "\nr: recompute / rebuild WINC PLL tables"
                        "\n> ");
    flush_serial_input();
    set_state(CMD_TASK_STATE_AWAIT_COMMAND);
  } break;

  case CMD_TASK_STATE_AWAIT_COMMAND: {
    // Here waiting for user input
    uint8_t buf[1];
    size_t n_read;

    n_read = SYS_CONSOLE_Read(SYS_CONSOLE_DEFAULT_INSTANCE, buf, sizeof(buf));

    if (n_read < 0) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nError while reading from console");
      set_state(CMD_TASK_STATE_ERROR);
    } else if (n_read > 0) {
      switch (buf[0]) {
      case 'h':
        set_state(CMD_TASK_STATE_PRINTING_HELP);
        break;
      case 'e':
        line_reader_start();
        SYS_CONSOLE_MESSAGE("extract WINC firmware into filename: ");
        set_state(CMD_TASK_STATE_START_EXTRACTING);
        break;
      case 'u':
        line_reader_start();
        SYS_CONSOLE_MESSAGE("update WINC firmware from filename: ");
        set_state(CMD_TASK_STATE_START_UPDATING);
        break;
      case 'c':
        line_reader_start();
        SYS_CONSOLE_MESSAGE("compare WINC firmware against filename: ");
        set_state(CMD_TASK_STATE_START_COMPARING);
        break;
      case 'r':
        SYS_CONSOLE_MESSAGE("recompute / rebuild WINC PLL tables");
        set_state(CMD_TASK_STATE_START_REBUILDING);
        break;
      default:
        SYS_CONSOLE_PRINT("\nUnrecognized command '%c'", buf[0]);
        set_state(CMD_TASK_STATE_PRINTING_HELP);
      }
    } else {
      // no bytes read -- remain in this state.
    }
  } break;

  case CMD_TASK_STATE_START_EXTRACTING: {
    line_reader_step();

    if (line_reader_has_error()) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\ncould not read filename");
      set_state(CMD_TASK_STATE_PRINTING_HELP);  // restart...

    } else if (line_reader_succeeded()) {
      const char *filename = line_reader_get_line();
      SYS_CONSOLE_PRINT("\nExtracting WINC firmware into %s", filename);
      winc_cloner_extract(filename);
      set_state(CMD_TASK_STATE_PRINTING_HELP);

    } else {
      // remain in this state until line_reader completes.
    }
  } break;

  case CMD_TASK_STATE_START_UPDATING: {
    line_reader_step();

    if (line_reader_has_error()) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\ncould not read filename");
      set_state(CMD_TASK_STATE_PRINTING_HELP);  // restart...

    } else if (line_reader_succeeded()) {
      const char *filename = line_reader_get_line();
      SYS_CONSOLE_PRINT("\nUpdating WINC firmware from %s", filename);
      winc_cloner_update(filename);
      set_state(CMD_TASK_STATE_PRINTING_HELP);

    } else {
      // remain in this state until line_reader completes.
    }
  } break;

  case CMD_TASK_STATE_START_COMPARING: {
    line_reader_step();

    if (line_reader_has_error()) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\ncould not read filename");
      set_state(CMD_TASK_STATE_PRINTING_HELP);  // restart...

    } else if (line_reader_succeeded()) {
      const char *filename = line_reader_get_line();
      SYS_CONSOLE_PRINT("\nComparing WINC firmware against %s", filename);
      winc_cloner_compare(filename);
      set_state(CMD_TASK_STATE_PRINTING_HELP);

    } else {
      // remain in this state until line_reader completes.
    }
  } break;

  case CMD_TASK_STATE_START_REBUILDING: {
    // Arrive here to rebuild / repair the PLL tables based on the gain tables.
    winc_cloner_rebuild_pll();
    set_state(CMD_TASK_STATE_PRINTING_HELP);
  } break;

  case CMD_TASK_STATE_ERROR: {
    // here on error state
  } break;

  } // switch
}

bool cmd_task_has_error(void) {
  return s_cmd_task_ctx.state == CMD_TASK_STATE_ERROR;
}

// *****************************************************************************
// Private (static) code

static void set_state(cmd_task_state_t state) {
  if (s_cmd_task_ctx.state != state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "%s => %s",
                    state_name(s_cmd_task_ctx.state),
                    state_name(state));
    s_cmd_task_ctx.state = state;
  }
}

static const char *state_name(cmd_task_state_t state) {
  SYS_ASSERT(state < N_STATES, "cmd_task_state_t out of bounds");
  return s_state_names[state];
}

static void flush_serial_input(void) {
  char ch;
  while (SYS_CONSOLE_Read(SYS_CONSOLE_DEFAULT_INSTANCE, &ch, sizeof(ch)) > 0) {
    // eat any stray characters in the input buffer.
  }
}

// *****************************************************************************
// End of file
