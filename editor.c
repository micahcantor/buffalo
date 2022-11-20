#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rope.h"
 
int main(int argc, char** argv) {
    // Initialize the ncurses window
  /* WINDOW* mainwin = initscr();
  if (mainwin == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(2);
  }

  noecho();                // Don't print keys when pressed
  keypad(mainwin, true);   // Support arrow keys
  nodelay(mainwin, true);  // Non-blocking keyboard access */

  if (argc != 2) {
    fprintf(stderr, "Usage: editor <file_name>.\n");
    exit(1);
  }

  char* file_name = argv[1];

  printf("Editing file: '%s'\n", file_name);

  rope_t rope;
  rope_init(&rope);
  rope_from_string(&rope, "Hello, World!", 13);

  rope_pair_t split = rope_split(&rope, 7);
  rope_print(split.left);
  rope_print(split.right);

  rope_destroy(&rope);

  /* // Clean up window
  delwin(mainwin);
  endwin(); */

  return 0;
}