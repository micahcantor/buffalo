#pragma once

#include <stdbool.h>
#include <stdio.h>
#include "gap_buffer.h"

typedef struct {
  gap_buffer_t* gb;
  const char* file_path;
  FILE* input;
  int row;
  int col;
  bool running;
  bool saved;
} buffalo_state_t;

void init_buffalo_state(buffalo_state_t* bs, const char* file_path, FILE* input, gap_buffer_t* gb);