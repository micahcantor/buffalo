#include "ui.h"
#include "buffalo_state.h"
#include "row.h"
#include <form.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// The timeout for input
#define INPUT_TIMEOUT_MS 10

// The ncurses forms code is loosely based on the first example at
// http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/forms.html

// The fields array for the editor field. Must end in NULL
FIELD* editor_fields[2];

// The form that holds the editor field
FORM* editor_form;

// The handle for the UI thread
pthread_t ui_thread;

// A lock to protect the entire UI
pthread_mutexattr_t ui_lock_attr;
pthread_mutex_t ui_lock;

// Ctrl-modified keys can be extracted with a bitmask. See https://stackoverflow.com/a/39960443
#define CTRL(c) ((c) & 037)

void ui_init(buffalo_state_t* bs) {
  // Initialize curses
  initscr();
  raw();
  noecho();
  timeout(INPUT_TIMEOUT_MS);

  // Get the number of rows and columns in the terminal display
  int rows;
  int cols;
  getmaxyx(stdscr, rows, cols);  // This uses a macro to modify rows and cols

  // Calculate the height of the display field
  int editor_height = rows - 1;

  // Create the editor window
  // height, width, start row, start col, overflow buffer lines, buffers
  editor_fields[0] = new_field(editor_height, cols, 0, 0, 0, 0);
  editor_fields[1] = NULL;

  // Grow the editor field buffer as needed
  field_opts_off(editor_fields[0], O_STATIC);

  // Turn off word wrap (nice, but causes other problems)
  field_opts_off(editor_fields[0], O_WRAP);

  // Create the forms
  editor_form = new_form(editor_fields);

  // Display the form
  post_form(editor_form);
  refresh();

  // Initialize the UI lock
  pthread_mutexattr_init(&ui_lock_attr);
  pthread_mutexattr_settype(&ui_lock_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&ui_lock, &ui_lock_attr);

  // Display initial data
  pthread_mutex_lock(&ui_lock);
  for (int i = 0; i < bs->row_list->size; i++) {
    row_t row = bs->row_list->rows[i];
    for (int j = 0; j < row.size; j++) {
      form_driver(editor_form, row.chars[j]);
    }
    if (i != bs->row_list->size - 1) {
      form_driver(editor_form, REQ_NEW_LINE);
    }
  }
  pthread_mutex_unlock(&ui_lock);

}

static void quit(buffalo_state_t* bs) {
  if (bs->saved) {
    ui_exit(bs);
  } else {
    // TODO: display message to confirm quit w/o save
  }
}

static void save(buffalo_state_t* bs) {
  // Clear contents of the file, reset cursor to beginning
  bs->input = freopen(NULL, "w+", bs->input);
  if (bs->input == NULL) {
    perror("Unable to clear input file");
    exit(2);
  }

  // Write a line in the file for each row
  for (int i = 0; i < bs->row_list->size; i++) {
    row_t row = bs->row_list->rows[i];
    fwrite(row.chars, row.size, sizeof(char), bs->input);

    // Write a newline character unless we're at the last row
    if (i != bs->row_list->size - 1) {
      fwrite("\n", 1, sizeof(char), bs->input);
    }
  }

  // Set saved flag
  bs->saved = true;
}

static inline void arrow_key(buffalo_state_t* bs) {
  int ch1 = getch();
  if (ch1 == '[') {
    int ch2 = getch();
    if (ch2 == 'A') {
      form_driver(editor_form, REQ_PREV_LINE);
    } else if (ch2 == 'B') {
      form_driver(editor_form, REQ_NEXT_LINE);
    } else if (ch2 == 'C') {
      form_driver(editor_form, REQ_NEXT_CHAR);
    } else if (ch2 == 'D') {
      form_driver(editor_form, REQ_PREV_CHAR);
    }
  }
}

static inline void edit(int ch, buffalo_state_t* bs) {
  // Set saved flag
  bs->saved = false;

  // Get row and col from ncurses
  int row, col;
  getyx(stdscr, row, col);

  // Pointer to current row
  row_t* current_row = &bs->row_list->rows[row];

  if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
    // Update UI
    form_driver(editor_form, REQ_DEL_PREV);
    
    // If not at beginning of line, delete previous character
    if (col > 0) {
      row_delete_at(current_row, col - 1);
    } else { // Otherwise delete current line
      row_t* prev_row = &bs->row_list->rows[row - 1];

      // Copy from current row into the end of the previous row
      row_insert_chars_at(prev_row, current_row->chars, current_row->size, prev_row->size);
      row_list_delete_at(bs->row_list, row);
    }
  } else if (ch == KEY_ENTER || ch == '\n') {
    // Update UI
    form_driver(editor_form, REQ_NEW_LINE);
    
    // Initialize new row
    row_t new_row;
    row_init(&new_row);

    // If at the beginning of a line, just insert an empty line above
    if (col == 0) {
      row_list_insert_at(bs->row_list, new_row, row);
    } else { // Otherwise we have to split the current line
      // Insert the characters after the cursor into the new row
      row_insert_chars_at(&new_row, current_row->chars + col, current_row->size - col, 0);

      // Insert new row after current
      row_list_insert_at(bs->row_list, new_row, row + 1);

      // Update size of current row. Don't use current_row pointer because of reallocation
      bs->row_list->rows[row].size = col;
    }
  } else {
    // Update UI, insert character
    form_driver(editor_form, ch);
    row_insert_at(current_row, ch, col);
  }
}

/**
 * Run the main UI loop. This function will only return the UI is exiting.
 */
void ui_run(buffalo_state_t* bs) {
  // Loop as long as the program is running
  while (bs->running) {
    // Get a character
    int ch = getch();

    // If there was no character, try again
    if (ch == -1) continue;

    // There was some character. Lock the UI
    pthread_mutex_lock(&ui_lock);

    if (ch == CTRL('Q')) { // quit
      quit(bs);
    } else if (ch == CTRL('S')) { // save
      save(bs);
    } else if (ch == '\x1b') { // escape to start arrow key
      arrow_key(bs);
    } else { // edit
      edit(ch, bs);
    }

    // Unlock the UI
    pthread_mutex_unlock(&ui_lock);
  }
}

/**
 * Stop the user interface and clean up.
 */
void ui_exit(buffalo_state_t* bs) {
  // Block access to the UI
  pthread_mutex_lock(&ui_lock);

  // The UI is not running
  bs->running = false;

  // Clean up
  unpost_form(editor_form);
  free_form(editor_form);
  free_field(editor_fields[0]);
  endwin();

  // Unlock the UI
  pthread_mutex_unlock(&ui_lock);
}
