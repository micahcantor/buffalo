#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "row.h"
#include "ui.h"
#include "buffalo_state.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN_INIT_SIZE 16

FILE* load_file(row_list_t* row_list, const char* file_path) {
  // Try to open input file
  FILE* input = fopen(file_path, "r+");
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

  char* line;
  char* contents_temp = contents; // strsep mangles this pointer, so save it
  while ((line = strsep(&contents_temp, "\n")) != NULL) {
    row_t row;
    row_init(&row);
    row_insert_chars_at(&row, line, strlen(line), 0);
    row_list_append(row_list, row);
  }

  free(contents);

  return input;
}
 
int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: buffalo <file_name>.\n");
    exit(1);
  }

  char* file_path = argv[1];

  // Initialize gap buffers
  row_list_t row_list;
  row_list_init(&row_list);
  FILE* input = load_file(&row_list, file_path);
  
  // Initialize program state
  buffalo_state_t bs;
  init_buffalo_state(&bs, file_path, input, &row_list);

  // Initialize the ui
  ui_init(&bs);

  // Run the ui, continuing until ui_exit is called somewhere
  ui_run(&bs);

  // Clean up
  row_list_destroy(&row_list);
  fclose(input);
  
  return 0;
}