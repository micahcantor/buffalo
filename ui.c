#include "ui.h"
#include "buffalo_state.h"
#include "row.h"
#include <curses.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define HEADER_HEIGHT 1
#define FOOTER_HEIGHT 1

// Size of terminal window
int num_rows, num_cols;

// height of editor
int editor_height;

// name of the file
char* file_name;

// Ctrl-modified keys can be extracted with a bitmask. See https://stackoverflow.com/a/39960443
#define CTRL(c) ((c) & 037)

/* Draw the entire editor, including header, footer and file contents. */
static void display_editor(buffalo_state_t* bs) {
  // Turn of cursor while redrawing display
  curs_set(0);

  // Draw header/footer split
  for (int i = 0; i < num_cols; i++) {
    mvaddch(HEADER_HEIGHT, i, '-');
    mvaddch(num_rows - FOOTER_HEIGHT - 1, i, '-');
  }

  // Display header
  mvprintw(0, num_cols - strlen(file_name), "%s", file_name);
  mvprintw(0, num_cols / 2 - 7, "buffalo");
  
  // Print line/col
  mvprintw(0, 0, "Ln %d, Col %d", bs->row + HEADER_HEIGHT, bs->col + 1);

  // Display contents of file. Start at scroll offset up the last row and height of editor
  for (int i = bs->scroll_offset; i < bs->row_list->size && i < editor_height + bs->scroll_offset; i++) {
    int editor_row = i + HEADER_HEIGHT + 1 - bs->scroll_offset;
    row_t row = bs->row_list->rows[i];
    for (int j = 0; j < row.size; j++) {
      mvaddch(editor_row, j, row.chars[j]);
    }
  }

  // Turn back on cursor
  curs_set(1);
}

/* Clear each line of the editor, except the last one where messages are put */
static void clear_editor() {
  for (int i = 0; i < num_rows - 1; i++) {
    move(i, 0);
    clrtoeol();
  }
}

/* Run a command in the shell. Returns the exit status of the command. */
static int run_command(const char* command) {
  // Create a child process
  pid_t child_id = fork();
  if (child_id == -1) {
    perror("fork failed");
    exit(EXIT_FAILURE);
  }

  // Execute command in child
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

    return 0;
  } else { // wait in parent
    int status;
    pid_t rc = wait(&status);
    if (rc == -1) {
      perror("wait failed");
      exit(EXIT_FAILURE);
    }

    return WEXITSTATUS(status);
  }
}

/* Run the configured build command, and print a message in the footer */
static void build(buffalo_state_t* bs) {
  if (bs->config->build_command != NULL) {
    int rc = run_command(bs->config->build_command);
    mvprintw(num_rows - 1, 0, "Build finished with status %d", rc);
  } else {
    mvprintw(num_rows - 1, 0, "No configured build command found");
  }
}

/* Run the configured test command, and print a message in the footer */
static void test(buffalo_state_t* bs) {
  if (bs->config->test_command != NULL) {
    int rc = run_command(bs->config->test_command);
    mvprintw(num_rows - 1, 0, "Tests finished with status %d", rc);
  } else {
    mvprintw(num_rows - 1, 0, "No configured test command found");
  }
}

/* Quit the program, prompting the user to confirm if unsaved. */
static void quit(buffalo_state_t* bs) {
  if (bs->saved) {
    ui_exit(bs);
  } else {
    mvprintw(num_rows - 1, 0, "Quit without saving? (y/n)");
    int ch = getch();
    if (ch == 'y') 
      ui_exit(bs);
    else {
      // Clear the message from the footer
      move(num_rows - 1, 0);
      clrtoeol();
    }
  }
}

/* Write the contents of the file to disk. */
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

/* Handle arrow key cursor movement from user. */
static inline void arrow_key(buffalo_state_t* bs) {
  int ch1 = getch();
  if (ch1 == '[') { // first char in sequence is '['
    int ch2 = getch();
    if (ch2 == 'A') { // up arrow
      // Prevent moving up past first row
      if (bs->row > 0) {
        bs->col = 0;
        bs->row--;
      }

      // Decrement scroll if at the top of the screen
      if (bs->row == bs->scroll_offset - 1) {
        bs->scroll_offset--;
      }
    } else if (ch2 == 'B') { // down arrow
      // stop from moving below last line
      if (bs->row < bs->row_list->size - 1) { 
        bs->col = 0;
        bs->row++;
      }

      // Increment scroll if at bottom of the screen
      if (bs->row == editor_height + bs->scroll_offset) {
        bs->scroll_offset++;
      }
    } else if (ch2 == 'C') { // right arrow
      int row_size = bs->row_list->rows[bs->row].size;
      // If at the end of the last line, do nothing
      if (bs->col == row_size && bs->row == bs->row_list->size - 1) return;

      // If at the end of a line, skip to the next line.
      if (bs->col == row_size) {
        bs->col = 0;
        bs->row++;
      } else { // otherwise just move forward once
        bs->col++;
      }
    } else if (ch2 == 'D') { // left arrow
      // If at beginning of first line, do nothing
      if (bs->col == 0 && bs->row == 0) return;

      // If at the beginning of a line, jump to the end of the previous line.
      if (bs->col == 0 && bs->row > 0) {
        size_t prev_row_size = bs->row_list->rows[bs->row - 1].size;
        bs->col = prev_row_size;
        bs->row--;
      } else { // Otherwise just move back once
        bs->col--;
      }
    }
  }
}

/* Handle backspace input from user. */
static void backspace(buffalo_state_t* bs, row_t* current_row) {
  // If not at beginning of line, delete previous character
  if (bs->col > 0) {
    row_delete_at(current_row, bs->col - 1);
    bs->col--;
  } else if (bs->row > 0) { // Unless on the first line, delete this row
    row_t* prev_row = &bs->row_list->rows[bs->row - 1];
    bs->col = prev_row->size;

    // Copy from current row into the end of the previous row
    row_insert_chars_at(prev_row, current_row->chars, current_row->size, prev_row->size);
    row_list_delete_at(bs->row_list, bs->row);

    bs->row--; // move back a row
  }
}

/* Handle enter key input from user */
static void enter(buffalo_state_t* bs, row_t* current_row) {
  // Initialize new row
  row_t new_row;
  row_init(&new_row);
  
  // If at the beginning of a line, just insert an empty line above
  if (bs->col == 0) {
    row_list_insert_at(bs->row_list, new_row, bs->row);
  } else { // Otherwise we have to split the current line
    // Insert the characters after the cursor into the new row
    row_insert_chars_at(&new_row, current_row->chars + bs->col, current_row->size - bs->col, 0);

    // Insert new row after current
    row_list_insert_at(bs->row_list, new_row, bs->row + 1);

    // Update size of current row. Don't use current_row pointer because of reallocation
    bs->row_list->rows[bs->row].size = bs->col;

    // Move cursor to beginning of line
    bs->col = 0;
  }

  bs->row++; // move to next row
}

/* Handle an edit performed by the user */
static void edit(int ch, buffalo_state_t* bs) {
  // Set saved flag
  bs->saved = false;

  // Pointer to current row
  row_t* current_row = &bs->row_list->rows[bs->row];

  if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
    backspace(bs, current_row);
  } else if (ch == KEY_ENTER || ch == '\n') {
    enter(bs, current_row);
  } else {
    row_insert_at(current_row, ch, bs->col);
    bs->col++;
  }
}

/* Initialize the UI, displaying initial contents. */
void ui_init(buffalo_state_t* bs) {
  // Initialize curses
  initscr();
  raw();
  noecho();

  // Get the number of rows and columns in the terminal display
  getmaxyx(stdscr, num_rows, num_cols);

  // Calculate editor height
  editor_height = num_rows - HEADER_HEIGHT - FOOTER_HEIGHT - 2;

  // Get the name of the file from the path
  file_name = basename((char*)bs->file_path);

  display_editor(bs);
  move(bs->row + HEADER_HEIGHT + 1, bs->col);
}

/* Run the main UI loop. Intended to be called by pthread_create. */
void* ui_run(void* arg) {
  // Cast arg to buffalo state pointer
  buffalo_state_t* bs = (buffalo_state_t*)arg;

  // Loop as long as the program is running
  while (bs->running) {
    // Get a character
    int ch = getch();

    // If there was no character, try again
    if (ch == -1) continue;

    // Handle user input
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

    // Clear and redraw the screen
    clear_editor();
    display_editor(bs);

    // Move back to current row/col
    move(bs->row + HEADER_HEIGHT + 1 - bs->scroll_offset, bs->col);
  }

  return NULL;
}

/* Stop the UI, clean up. */
void ui_exit(buffalo_state_t* bs) {
  bs->running = false;
  endwin();
}
