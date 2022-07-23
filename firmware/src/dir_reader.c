/**
 * @file dir_reader.c
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

#include "dir_reader.h"

#include "app.h"
#include "definitions.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define MAX_FILENAME_LENGTH 80
#define MAX_IMG_FILENAMES 20

#define IMAGE_EXTENSION ".wimg"

#define STATES(M)                                                              \
  M(DIR_READER_STATE_IDLE)                                                     \
  M(DIR_READER_STATE_OPENING_DIRECTORY)                                        \
  M(DIR_READER_STATE_READING_DIRECTORY)                                        \
  M(DIR_READER_STATE_CLOSING_DIRECTORY)                                        \
  M(DIR_READER_STATE_COMPLETE)                                                 \
  M(DIR_READER_STATE_ERROR)

#define EXPAND_STATE_IDS(_name) _name,
typedef enum { STATES(EXPAND_STATE_IDS) } dir_reader_state_t;

typedef struct {
  dir_reader_state_t state;
  dir_reader_callback_fn callback_fn;
  uintptr_t callback_arg;
  SYS_FS_HANDLE dir_handle;
  uint8_t file_count; // # of .img files found
} dir_reader_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the internal state.
 */
static void set_state(dir_reader_state_t state);

/**
 * @brief Return the name of the given state.
 */
static const char *state_name(dir_reader_state_t state);

/**
 * @brief Set the state to final_state and invoke callback.
 */
static void endgame(dir_reader_state_t final_state);

/**
 * @brief Return true if the last chars of str equal suffix.
 */
static bool string_ends_with(const char *str, const char *suffix);

// *****************************************************************************
// Private (static) storage

#define EXPAND_STATE_NAMES(_name) #_name,
static const char *s_state_names[] = {STATES(EXPAND_STATE_NAMES)};

#define N_STATES (sizeof(s_state_names) / sizeof(s_state_names[0]))

char s_filenames[MAX_IMG_FILENAMES * MAX_FILENAME_LENGTH];

char s_filename[MAX_FILENAME_LENGTH]; // to hold stat's long file names

static dir_reader_ctx_t s_dir_reader_ctx;

// *****************************************************************************
// Public code

void dir_reader_init(void) {
  s_dir_reader_ctx.state = DIR_READER_STATE_IDLE;
  s_dir_reader_ctx.dir_handle = SYS_FS_HANDLE_INVALID;
}

/**
 * @brief Step the demo task internal state.  Called frequently.
 */
void dir_reader_step(void) {
  switch (s_dir_reader_ctx.state) {
  case DIR_READER_STATE_IDLE: {
    // wait here for a call to dirlist_read_directory()
  } break;

  case DIR_READER_STATE_OPENING_DIRECTORY: {
    // here when dirlist_read_directory() has been called.
    s_dir_reader_ctx.file_count = 0;
    memset(s_filenames, 0, sizeof(s_filenames));

    s_dir_reader_ctx.dir_handle = SYS_FS_DirOpen(SD_MOUNT_NAME "/");
    if (s_dir_reader_ctx.dir_handle != SYS_FS_HANDLE_INVALID) {
      set_state(DIR_READER_STATE_READING_DIRECTORY);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nUnable to open directory %s", SD_MOUNT_NAME "/");
      endgame(DIR_READER_STATE_ERROR);
    }
  } break;

  case DIR_READER_STATE_READING_DIRECTORY: {
    // remain in this state until all directory entries are read and printed
    SYS_FS_FSTAT stat;
    stat.lfname = s_filename;
    stat.lfsize = sizeof(s_filename);

    if (SYS_FS_DirRead(s_dir_reader_ctx.dir_handle, &stat) ==
        SYS_FS_RES_FAILURE) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nUnable to read directory %s", SD_MOUNT_NAME "/");
      endgame(DIR_READER_STATE_ERROR);

    } else if ((stat.lfname[0] == '\0') && (stat.fname[0] == '\0')) {
      set_state(DIR_READER_STATE_CLOSING_DIRECTORY);

    } else if (string_ends_with(stat.fname, IMAGE_EXTENSION)) {
      if (s_dir_reader_ctx.file_count >= MAX_IMG_FILENAMES) {
        SYS_DEBUG_PRINT(
            SYS_ERROR_ERROR, "\ndirectory full, skipping %s", stat.fname);
      } else {
        // read succeeded and has a ".img" suffix.  Record it.
        strncpy((char *)dir_reader_filename_ref(s_dir_reader_ctx.file_count++),
                (const char *)stat.fname,
                MAX_FILENAME_LENGTH);
      }
    }
    // remain in this state to read more filenames
  } break;

  case DIR_READER_STATE_CLOSING_DIRECTORY: {
    if (SYS_FS_DirClose(s_dir_reader_ctx.dir_handle) != SYS_FS_RES_SUCCESS) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nClosing directory %s failed", SD_MOUNT_NAME "/");
    }
    s_dir_reader_ctx.dir_handle = SYS_FS_HANDLE_INVALID;
    endgame(DIR_READER_STATE_COMPLETE);
  } break;

  case DIR_READER_STATE_COMPLETE: {
    // here on error state
  } break;

  case DIR_READER_STATE_ERROR: {
    // here on error state
  } break;
  } // switch
}

void dir_reader_set_callback(dir_reader_callback_fn callback_fn,
                             uintptr_t callback_arg) {
  s_dir_reader_ctx.callback_fn = callback_fn;
  s_dir_reader_ctx.callback_arg = callback_arg;
}

void dir_reader_read_directory(void) {
  set_state(DIR_READER_STATE_OPENING_DIRECTORY);
}

uint8_t dir_reader_filename_count(void) { return s_dir_reader_ctx.file_count; }

const char *dir_reader_filename_ref(uint8_t idx) {
  if (idx < s_dir_reader_ctx.file_count) {
    return &s_filenames[idx * MAX_FILENAME_LENGTH];
  } else {
    return NULL;
  }
}

bool dir_reader_is_idle(void) {
  return s_dir_reader_ctx.state == DIR_READER_STATE_IDLE;
}

bool dir_reader_is_complete(void) {
  return s_dir_reader_ctx.state == DIR_READER_STATE_COMPLETE;
}

bool dir_reader_has_error(void) {
  return s_dir_reader_ctx.state == DIR_READER_STATE_ERROR;
}

// *****************************************************************************
// Private (static) code

static void set_state(dir_reader_state_t state) {
  if (s_dir_reader_ctx.state != state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "%s => %s",
                    state_name(s_dir_reader_ctx.state),
                    state_name(state));
    s_dir_reader_ctx.state = state;
  }
}

static const char *state_name(dir_reader_state_t state) {
  SYS_ASSERT(state < N_STATES, "dir_reader_state_t out of bounds");
  return s_state_names[state];
}

static void endgame(dir_reader_state_t final_state) {
  set_state(final_state);
  if (s_dir_reader_ctx.callback_fn) {
    s_dir_reader_ctx.callback_fn(s_dir_reader_ctx.callback_arg);
  }
}

static bool string_ends_with(const char *str, const char *suffix) {
  if (str == NULL) {
    return false;
  } else if (suffix == NULL) {
    return false;
  }
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (str_len < suffix_len) {
    return false;
  } else {
    // compare strings starting at suffix_len from the end of str.
    return strcmp(&str[str_len - suffix_len], suffix) == 0;
  }
}

// *****************************************************************************
// End of file
