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

#define CTRL(c) ((c) & 037)

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
  for(int i = 0; i < bs->gb->left; i++) {
    form_driver(editor_form, bs->gb->data[i]);
  }
  for (int i = bs->gb->right; i < bs->gb->size; i++) {
    form_driver(editor_form, bs->gb->data[i]);
  }
  pthread_mutex_unlock(&ui_lock);
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
      if (bs->saved) {
        ui_exit(bs);
      } else {
        // TODO: display message to confirm quit w/o save
      }
    } else if (ch == CTRL('S')) { // save
      char* data = malloc(bs->gb->capacity);
      gap_buffer_data(bs->gb, data);
      fwrite(data, bs->gb->capacity, 1, bs->input);
      free(data);
      bs->saved = true;
    } else { // non-ctrl key
      bs->saved = false;
      if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
        form_driver(editor_form, REQ_DEL_PREV);
        delete_char(bs->gb);
      } else if (ch == KEY_ENTER || ch == '\n') {
        form_driver(editor_form, REQ_NEW_LINE);
        insert_char(bs->gb, '\n');
      } else {
        form_driver(editor_form, ch);
        insert_char(bs->gb, ch);
      }
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
