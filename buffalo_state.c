#include <stdio.h>
#include <stdbool.h>
#include "buffalo_state.h"
#include "row.h"

void init_buffalo_state(buffalo_state_t* bs, const char* file_path, FILE* input, row_list_t* row_list) {
  bs->row_list = row_list;
  bs->file_path = file_path;
  bs->input = input;
  bs->running = true;
  bs->saved = true;
}