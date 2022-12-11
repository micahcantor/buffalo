#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gap_buffer.h"

/* Single gap buffer */

void init_gap_buffer(gap_buffer_t* gb, size_t size) {
  gb->data = malloc(size);
  gb->size = size;
  gb->capacity = 0;
  gb->left = 0; 
  gb->right = size;
}

void destroy_gap_buffer(gap_buffer_t* gb) {
  free(gb->data);
}

static void expand(gap_buffer_t* gb) {
  size_t new_size = gb->size * 2;
  size_t gap_size = new_size - gb->size;
  char* old_data = gb->data;
  char* new_data = malloc(new_size);
  memcpy(new_data, old_data, gb->left);
  memcpy(new_data + gb->right + gap_size, old_data + gb->right, gb->size - gb->right);
  free(old_data);
  gb->data = new_data;
  gb->size = new_size;
  gb->right += gap_size;
}

void cursor_forward(gap_buffer_t* gb) {
  if (gb->right < gb->size) {
    char c = gb->data[gb->right - 1];
    gb->data[gb->left] = c;
    gb->left++;
    gb->right++;
  }
}

void cursor_backward(gap_buffer_t* gb) {
  if (gb->left > 0) {
    char c = gb->data[gb->left - 1];
    gb->data[gb->right - 1] = c;
    gb->right--;
    gb->left--;
  }
}

void move_cursor(gap_buffer_t* gb, int distance) {
  if (distance > 0) {
    for (int i = 0; i < distance; i++)
      cursor_forward(gb);
  } else {
    for (int i = 0; i < -distance; i++)
      cursor_backward(gb);
  }
}

void move_cursor_to(gap_buffer_t* gb, int location) {
  move_cursor(gb, location - gb->left);
}

void insert_char(gap_buffer_t* gb, char c) {
  if (gb->left >= gb->right - 1) {
    expand(gb);
  }
  gb->data[gb->left] = c;
  gb->capacity++;
  gb->left++;
}

void insert_string(gap_buffer_t* gb, char* str) {
  int i = 0;
  while (str[i] != '\0') {
    insert_char(gb, str[i]);
    i++;
  }
}

void delete_char(gap_buffer_t* gb) {
  if (gb->left > 0) {
    gb->left--;
    gb->capacity--;
  }
}

void print_gap_buffer(gap_buffer_t* gb) {
  for (int i = 0; i < gb->left; i++) {
    printf("%c", gb->data[i]);
  }
  for (int i = gb->right; i < gb->size; i++) {
    printf("%c", gb->data[i]);
  }
  printf("\n");
}

void gap_buffer_data(gap_buffer_t* gb, char* data) {
  memcpy(data, gb->data, gb->left);
  memcpy(data + gb->left + 1, gb->data + gb->right, gb->size - gb->right);
}

/* List of gap buffers */

void gb_list_init(gb_list_t* gbs) {
  gbs->buffers = NULL;
  gbs->length = 0;
}

void gb_list_insert_at(gb_list_t* gbs, gap_buffer_t gb, size_t idx) {
  gbs->buffers = realloc(gbs->buffers, (gbs->length + 1) * sizeof(gap_buffer_t));
  memmove(gbs->buffers + idx + 1, gbs->buffers + idx, (gbs->length - idx) * sizeof(gap_buffer_t));
  gbs->buffers[idx] = gb;
  gbs->length++;
}

void gb_list_add(gb_list_t* gbs, gap_buffer_t gb) {
  gb_list_insert_at(gbs, gb, gbs->length);
}

void gb_list_delete(gb_list_t* gbs, size_t idx) {
  if (gbs == NULL || gbs->buffers == NULL || idx >= gbs->length) {
    return;
  }

  // Shift all elements after the deleted element to the left by one position.
  for (size_t i = idx; i < gbs->length - 1; i++) {
    destroy_gap_buffer(&gbs->buffers[i]);
    gbs->buffers[i] = gbs->buffers[i + 1];
  }

  // Decrement the length of the array by one.
  gbs->length--;
}

void gb_list_destroy(gb_list_t* gbs) {
  for (size_t i = 0; i < gbs->length; i++) {
    destroy_gap_buffer(&gbs->buffers[i]);
  }
  free(gbs->buffers);
}