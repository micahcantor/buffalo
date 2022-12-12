#pragma once

#include <string.h>

typedef struct {
  char* chars;
  size_t size;
} row_t;

typedef struct {
  row_t* rows;
  size_t size;
} row_list_t;

void row_init(row_t* row);

void row_insert_at(row_t* row, char c, size_t idx);

void row_insert_chars_at(row_t* row, char* chars, size_t num, size_t idx);

void row_delete_at(row_t* row, size_t idx);

void row_destroy(row_t* row);

void row_to_string(row_t* row, char* str);

void row_list_init(row_list_t* row_list);

void row_list_insert_at(row_list_t* row_list, row_t row, size_t idx);

void row_list_append(row_list_t* row_list, row_t row);

void row_list_delete_at(row_list_t* row_list, size_t idx);

void row_list_destroy(row_list_t* row_list);


