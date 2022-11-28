#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gap_buffer.h"

typedef struct {
  const char* file_path;
  int row;
  int col;
  bool running;
} buffalo_state_t;

// Global state
buffalo_state_t buffalo_state;

buffalo_state_t init_buffalo_state(const char* file_path) {
  buffalo_state_t buffalo_state;
  buffalo_state.file_path = file_path;
  buffalo_state.row = 0;
  buffalo_state.col = 0;
  buffalo_state.running = true;
  return buffalo_state;
}

/**
 * Convert a board row number to a screen position
 * \param   row   The board row number to convert
 * \return        A corresponding row number for the ncurses screen
 */
int screen_row(int row) {
  return 2 + row;
}

/**
 * Convert a board column number to a screen position
 * \param   col   The board column number to convert
 * \return        A corresponding column number for the ncurses screen
 */
int screen_col(int col) {
  return 2 + col;
}

WINDOW* init_display() {
  WINDOW* mainwin = initscr();
  if (mainwin == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(2);
  }

  noecho();                // Don't print keys when pressed
  keypad(mainwin, true);   // Support arrow keys
  nodelay(mainwin, true);  // Non-blocking keyboard access

  int num_rows, num_cols;
  getmaxyx(mainwin, num_rows, num_cols);

  // Print corners
  mvaddch(screen_row(-1), screen_col(-1), ACS_ULCORNER);
  mvaddch(screen_row(-1), screen_col(num_cols), ACS_URCORNER);
  mvaddch(screen_row(num_rows), screen_col(-1), ACS_LLCORNER);
  mvaddch(screen_row(num_rows), screen_col(num_cols), ACS_LRCORNER);

  // Print top and bottom edges
  for (int col = 0; col < num_cols; col++) {
    mvaddch(screen_row(-1), screen_col(col), ACS_HLINE);
    mvaddch(screen_row(num_rows), screen_col(col), ACS_HLINE);
  }

  // Print left and right edges
  for (int row = 0; row < num_rows; row++) {
    mvaddch(screen_row(row), screen_col(-1), ACS_VLINE);
    mvaddch(screen_row(row), screen_col(num_cols), ACS_VLINE);
  }

  // Refresh the display
  refresh();

  return mainwin;
}

FILE* load_file(gap_buffer_t* gb, const char* file_path) {
  // Try to open input file
  FILE* input = fopen(file_path, "a+");
  if (input == NULL) {
    perror("Unable to open input file");
    exit(1);
  }

  // Seek to the end of the file so we can get its size
  if (fseek(input, 0, SEEK_END) != 0) {
    perror("Unable to seek to end of file");
    exit(2);
  }

  // Get the size of the file
  size_t input_size = ftell(input);

  // Seek back to the beginning of the file
  if (fseek(input, 0, SEEK_SET) != 0) {
    perror("Unable to seek to beginning of file");
    exit(2);
  }

  char* contents = malloc(input_size + 1);
  contents[input_size] = '\0';

  // Read the file contents
  if (fread(contents, sizeof(char), input_size, input) != input_size) {
    fprintf(stderr, "Failed to read entire file\n");
    exit(2);
  }

  init_gap_buffer(gb, input_size);
  insert_string(gb, contents);

  free(contents);

  return input;
}

void read_input() {
  while(buffalo_state.running) {
    
  }
}
 
int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: editor <file_name>.\n");
    exit(1);
  }

  // Initialize the ncurses window
  WINDOW* window = init_display();

  char* file_path = argv[1];
  
  // Initialize program state
  buffalo_state = init_buffalo_state(file_path);

  // Initialize gap buffer
  gap_buffer_t gb;
  FILE* input = load_file(&gb, file_path);
  print_gap_buffer(&gb);

  move_cursor_to(&gb, 5);
  insert_char(&gb, '*');
  print_gap_buffer(&gb);

  read_input();

  // Clean up
  delwin(window);
  endwin();
  destroy_gap_buffer(&gb);
  fclose(input);
  
  return 0;
}

/* TODO: setup ncurses, display contents */
/* TODO: handle user input */