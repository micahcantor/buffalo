#include <stdio.h>
#include <stdbool.h>
#include "buffalo_state.h"
#include "gap_buffer.h"

void init_buffalo_state(buffalo_state_t* bs, const char* file_path, FILE* input, gap_buffer_t* gb) {
  bs->gb = gb;
  bs->file_path = file_path;
  bs->input = input;
  bs->row = 0;
  bs->col = 0;
  bs->running = true;
  bs->saved = true;
}