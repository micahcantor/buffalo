#pragma once

#include <stdbool.h>
#include <stdio.h>
#include "row.h"

typedef struct {
  row_list_t* row_list;
  const char* file_path;
  FILE* input;
  bool running;
  bool saved;
} buffalo_state_t;

void init_buffalo_state(buffalo_state_t* bs, const char* file_path, FILE* input, row_list_t* row_list);