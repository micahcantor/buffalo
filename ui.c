#include "ui.h"
#include "buffalo_state.h"
#include "row.h"
#include <form.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>

// The timeout for input
#define INPUT_TIMEOUT_MS 10

#define HEADER_HEIGHT 1
#define FOOTER_HEIGHT 1

// The ncurses forms code is loosely based on the first example at
// http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/forms.html

// The fields array for the editor field. Must end in NULL
FIELD* editor_fields[2];

// Fields array for the display fields.
FIELD* display_fields[3];

// The form that holds the editor field
FORM* editor_form;

// Form that holds the header field
FORM* header_form;

// Form that holds footer field
FORM* footer_form;

// The handle for the UI thread
pthread_t ui_thread;

// Size of terminal window
int num_rows, num_cols;

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
  getmaxyx(stdscr, num_rows, num_cols);  // This uses a macro to modify rows and cols

  // Calculate the height of the display field
  int editor_height = num_rows - HEADER_HEIGHT - FOOTER_HEIGHT - 2;

  // Create the editor window
  // height, width, start row, start col, overflow buffer lines, buffers
  editor_fields[0] = new_field(editor_height, num_cols, HEADER_HEIGHT + 1, 0, 0, 0);
  editor_fields[1] = NULL;

  // Create header/footer display fields
  display_fields[0] = new_field(HEADER_HEIGHT, num_cols, 0, 0, 0, 0);
  display_fields[1] = new_field(FOOTER_HEIGHT, num_cols, num_rows - FOOTER_HEIGHT, 0, 0, 0);
  display_fields[2] = NULL;

  // Grow the editor field buffer as needed
  field_opts_off(editor_fields[0], O_STATIC);

  // Don't advance to the next field automatically when using the input field
  field_opts_off(editor_fields[0], O_AUTOSKIP);

  // Turn off word wrap (nice, but causes other problems)
  field_opts_off(editor_fields[0], O_WRAP);
  field_opts_off(display_fields[0], O_WRAP);
  field_opts_off(display_fields[1], O_WRAP);

  // Create the forms
  editor_form = new_form(editor_fields);
  header_form = new_form(display_fields);
  footer_form = new_form(display_fields + 1);

  // Display the forms
  post_form(header_form);
  post_form(footer_form);
  post_form(editor_form);
  refresh();

  // Draw header/footer split
  for (int i = 0; i < num_cols; i++) {
    mvprintw(HEADER_HEIGHT, i, "-");
    mvprintw(num_rows - FOOTER_HEIGHT - 1, i, "-");
  }

  // Initialize the UI lock
  pthread_mutexattr_init(&ui_lock_attr);
  pthread_mutexattr_settype(&ui_lock_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&ui_lock, &ui_lock_attr);

  // Display initial data
  pthread_mutex_lock(&ui_lock);

  // Display header
  char* file_name = basename((char*)bs->file_path);
  mvprintw(0, num_cols - strlen(file_name), "%s", file_name);
  mvprintw(0, num_cols / 2 - 7, "buffalo");
  
  // Display initial contents of file
  for (int i = 0; i < bs->row_list->size; i++) {
    row_t row = bs->row_list->rows[i];
    for (int j = 0; j < row.size; j++) {
      mvprintw(i + HEADER_HEIGHT + 1, j, "%c", row.chars[j]);
    }
    if (i != bs->row_list->size - 1) {
      mvprintw(i + HEADER_HEIGHT + 1, row.size, "\n");
    }
  }
  move(HEADER_HEIGHT + 1, 0);
  pthread_mutex_unlock(&ui_lock);

}

static void run_command(const char* command) {
  // Create a child process
  pid_t child_id = fork();
  if (child_id == -1) {
    perror("fork failed");
    exit(EXIT_FAILURE);
  }

  // Execute build command in child
  if (child_id == 0) {
    // For now, silence stdout and stderr by sending them to /dev/null
    int null_fd = open("/dev/null", 0);
    dup2(null_fd, 1); // stdout
    dup2(null_fd, 2); // stderr

    // Pass command to sh with the -c (string input) flag
    int rc = execlp("/bin/sh", "/bin/sh", "-c", command, NULL);
    if (rc == -1) {
      perror("exec failed");
      exit(EXIT_FAILURE);
    }
  }
}

static void build(buffalo_state_t* bs) {
  if (bs->config->build_command != NULL) {
    run_command(bs->config->build_command);
  }
}

static void test(buffalo_state_t* bs) {
  if (bs->config->test_command != NULL) {
    run_command(bs->config->test_command);
  }
}

static void quit(buffalo_state_t* bs) {
  if (bs->saved) {
    ui_exit(bs);
  } else {
    // TODO: let user quit with unsaved work
    // mvprintw(num_rows - 1, 0, "Are you sure you want to quit with unsaved work?");
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

  // Flush changes to disk
  fflush(bs->input);

  // Set saved flag
  bs->saved = true;
}

static inline void arrow_key(buffalo_state_t* bs) {
  int ch1 = getch();
  if (ch1 == '[') {
    int ch2 = getch();
    if (ch2 == 'A') { // up arrow
      form_driver(editor_form, REQ_PREV_LINE);
    } else if (ch2 == 'B') { // down arrow
      if (bs->row != bs->row_list->size - 1) { // stop from moving below last line
        form_driver(editor_form, REQ_NEXT_LINE);
      }
    } else if (ch2 == 'C') { // right arrow
      int row_size = bs->row_list->rows[bs->row].size;
      // If at the end of the last line, do nothing
      if (bs->col == row_size && bs->row == bs->row_list->size - 1) {
        return;
      } // If at the end of a line, skip to the next line.
      else if (bs->col == row_size) {
        int distance = num_cols - row_size;
        for (int i = 0; i < distance; i++)
          form_driver(editor_form, REQ_NEXT_CHAR);
      } else { // otherwise just move forward once
        form_driver(editor_form, REQ_NEXT_CHAR);
      }
    } else if (ch2 == 'D') { // left arrow
      // If at the beginning of a line, jump to the end of the previous line.
      if (bs->col == 0 && bs->row > 0) {
        int distance = num_cols - bs->row_list->rows[bs->row - 1].size;
        for (int i = 0; i < distance; i++)
          form_driver(editor_form, REQ_PREV_CHAR);
      } else { // Otherwise just move back once
        form_driver(editor_form, REQ_PREV_CHAR);
      }
    }
  }
}

static inline void edit(int ch, buffalo_state_t* bs) {
  // Set saved flag
  bs->saved = false;

  // Pointer to current row
  row_t* current_row = &bs->row_list->rows[bs->row];

  if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
    // Update UI
    form_driver(editor_form, REQ_DEL_PREV);
    
    // If not at beginning of line, delete previous character
    if (bs->col > 0) {
      row_delete_at(current_row, bs->col - 1);
    } else { // Otherwise delete current line
      row_t* prev_row = &bs->row_list->rows[bs->row - 1];

      // Copy from current row into the end of the previous row
      row_insert_chars_at(prev_row, current_row->chars, current_row->size, prev_row->size);
      row_list_delete_at(bs->row_list, bs->row);
    }
  } else if (ch == KEY_ENTER || ch == '\n') {
    // Initialize new row
    row_t new_row;
    row_init(&new_row);

    // If at the beginning of a line, just insert an empty line above
    if (bs->col == 0) {
      row_list_insert_at(bs->row_list, new_row, bs->row);

      // Edge case: if on first row, use REQ_INS_LINE rather than REQ_NEW_LINE to update UI
      if (bs->row == 0) {
        form_driver(editor_form, REQ_INS_LINE);
      } else {
        form_driver(editor_form, REQ_NEW_LINE);
      }

    } else { // Otherwise we have to split the current line
      form_driver(editor_form, REQ_NEW_LINE);

      // Insert the characters after the cursor into the new row
      row_insert_chars_at(&new_row, current_row->chars + bs->col, current_row->size - bs->col, 0);

      // Insert new row after current
      row_list_insert_at(bs->row_list, new_row, bs->row + 1);

      // Update size of current row. Don't use current_row pointer because of reallocation
      bs->row_list->rows[bs->row].size = bs->col;
    }
  } else {
    // Update UI, insert character
    form_driver(editor_form, ch);
    row_insert_at(current_row, ch, bs->col);
  }
}

/**
 * Run the main UI loop. This function will only return the UI is exiting.
 */
void ui_run(buffalo_state_t* bs) {
  // Loop as long as the program is running
  while (bs->running) {
    // Lock the UI
    pthread_mutex_lock(&ui_lock);
    
    // Display line and col
    int row, col;
    getyx(stdscr, row, col);
    mvprintw(0, 0, "Ln %d, Col %d       ", row - HEADER_HEIGHT, col + 1);
    move(row, col); // move back to current row/col

    // Store row/col indices in state
    bs->row = row - HEADER_HEIGHT - 1;
    bs->col = col;

    // Get a character
    int ch = getch();

    // If there was no character, try again
    if (ch == -1) continue;

    if (ch == CTRL('Q')) { // quit
      quit(bs);
    } else if (ch == CTRL('S')) { // save
      save(bs);
    } else if (ch == CTRL('B')) { // build
      build(bs);
    } else if (ch == CTRL('T')) { // test
      test(bs);
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
  unpost_form(header_form);
  free_form(editor_form);
  free_form(header_form);
  free_field(editor_fields[0]);
  free_field(display_fields[0]);
  free_field(display_fields[1]);
  endwin();

  // Unlock the UI
  pthread_mutex_unlock(&ui_lock);
}
