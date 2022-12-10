#pragma once

#include <string.h>

typedef struct {
  char* data;
  size_t size;
  size_t capacity;
  size_t left;
  size_t right;
} gap_buffer_t;

typedef struct {
  gap_buffer_t* buffers;
  size_t length;
} gb_list_t;

void init_gap_buffer(gap_buffer_t* gb, size_t size);

void destroy_gap_buffer(gap_buffer_t* gb);

void cursor_forward(gap_buffer_t* gb);

void cursor_backward(gap_buffer_t* gb);

void move_cursor(gap_buffer_t* gb, int distance);

void move_cursor_to(gap_buffer_t* gb, int location);

void insert_char(gap_buffer_t* gb, char c);

void insert_string(gap_buffer_t* gb, char* str);

void delete_char(gap_buffer_t* gb);

void print_gap_buffer(gap_buffer_t* gb);

void gap_buffer_data(gap_buffer_t* gb, char* data);

void gb_list_init(gb_list_t* gbs);

void gb_list_insert_at(gb_list_t* gbs, gap_buffer_t gb, size_t idx);

void gb_list_add(gb_list_t* gbs, gap_buffer_t gb);

void gb_list_delete(gb_list_t* gbs, size_t idx);

void gb_list_destroy(gb_list_t* gbs);


