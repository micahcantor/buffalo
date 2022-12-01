#include "ui.h"
#include "buffalo_state.h"
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

/* If input is a control key squence, return the character pressed. Otherwise
   return the original. For example, ^Q -> Q. */
static char ctrl_key(char c) {
  const char* name = keyname(c);
  if (name[0] == '^') {
    return name[1];
  } else {
    return c;
  }
}

/**
 * Initialize the user interface and set up a callback function that should be
 * called every time there is a new message to send.
 *
 * \param callback  A function that should run every time there is new input.
 *                  The string passed to the callback should be copied if you
 *                  need to retain it after the callback function returns.
 */
void ui_init(buffalo_state_t* bs) {
  // Initialize curses
  initscr();
  raw();
  noecho();
  curs_set(0);
  timeout(INPUT_TIMEOUT_MS);

  // Get the number of rows and columns in the terminal display
  int rows;
  int cols;
  getmaxyx(stdscr, rows, cols);  // This uses a macro to modify rows and cols

  // Calculate the height of the display field
  int editor_height = rows - 1;

  // Create the larger message display window
  // height, width, start row, start col, overflow buffer lines, buffers
  editor_fields[0] = new_field(editor_height, cols, 0, 0, 0, 0);
  editor_fields[1] = NULL;

  // Grow the display field buffer as needed
  field_opts_off(editor_fields[0], O_STATIC);

  // Turn off word wrap (nice, but causes other problems)
  field_opts_off(editor_fields[0], O_WRAP);

  // Create the forms
  editor_form = new_form(editor_fields);

  // Display the forms
  post_form(editor_form);
  refresh();

  // Initialize the UI lock
  pthread_mutexattr_init(&ui_lock_attr);
  pthread_mutexattr_settype(&ui_lock_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&ui_lock, &ui_lock_attr);
}

/**
 * Run the main UI loop. This function will only return the UI is exiting.
 */
void ui_run(buffalo_state_t* bs) {
  // Loop as long as the UI is running
  while (bs->running) {
    // Get a character
    int ch = getch();

    // If there was no character, try again
    if (ch == -1) continue;

    // There was some character. Lock the UI
    pthread_mutex_lock(&ui_lock);

    // Handle input
    if (iscntrl(ch)) {
      switch (ctrl_key(ch)) {
        case 'Q':
          ui_exit(bs);
          break;
      }
    } else if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
      // Delete the last character when the user presses backspace
      form_driver(editor_form, REQ_DEL_PREV);
    } else {
      // Report normal input characters to the input field
      form_driver(editor_form, ch);
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
