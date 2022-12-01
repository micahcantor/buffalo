#pragma once

#include <stdbool.h>
#include "gap_buffer.h"

typedef struct {
  gap_buffer_t* gb;
  const char* file_path;
  int row;
  int col;
  bool running;
} buffalo_state_t;

void init_buffalo_state(buffalo_state_t* bs, const char* file_path, gap_buffer_t* gb);