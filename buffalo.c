#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "gap_buffer.h"
#include "ui.h"
#include "buffalo_state.h"

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
 
int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: buffalo <file_name>.\n");
    exit(1);
  }

  char* file_path = argv[1];

  // Initialize gap buffer
  gap_buffer_t gb;
  FILE* input = load_file(&gb, file_path);
  
  // Initialize program state
  buffalo_state_t bs;
  init_buffalo_state(&bs, file_path, &gb);

  // Initialize the ui
  ui_init(&bs);

  // Run the ui, continuing until ui_exit is called somewhere
  ui_run(&bs);

  // Clean up
  destroy_gap_buffer(&gb);
  fclose(input);
  
  return 0;
}