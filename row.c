#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "row.h"

/* Initialize a row_t pointer */
void row_init(row_t* row) {
  row->chars = NULL;
  row->size = 0;
}

/* Insert a character into a row at a given index */
void row_insert_at(row_t* row, char c, size_t idx) {
  row->chars = realloc(row->chars, row->size + 1);
  memmove(row->chars + idx + 1, row->chars + idx, row->size - idx);
  row->chars[idx] = c;
  row->size++;
}

/* Insert an array of characters into a row at a given index. Note that chars is not
  expected to be null-terminated. */
void row_insert_chars_at(row_t* row, char* chars, size_t num, size_t idx) {
  size_t row_idx = idx;
  for (int i = 0; i < num; i++) {
    row_insert_at(row, chars[i], row_idx);
    row_idx++;
  }
}

/* Delete a character from a row at a given index. */
void row_delete_at(row_t* row, size_t idx) {
  for (int i = idx; i < row->size - 1; i++) {
    row->chars[i] = row->chars[i + 1];
  }
  row->size--;
}

/* Free memory held by a row. */
void row_destroy(row_t* row) {
  free(row->chars);
}

/* Convert a row to a null-terminated string. Given pointer str must be at least
  one byte larger than row->chars. */
void row_to_string(row_t* row, char* str) {
  memcpy(str, row->chars, row->size);
  str[row->size] = '\0';
}

/* Initialize a row_list_t pointer */
void row_list_init(row_list_t* row_list) {
  row_list->rows = NULL;
  row_list->size = 0;
}

/* Insert a row into the row list */
void row_list_insert_at(row_list_t* row_list, row_t row, size_t idx) {
  row_list->rows = realloc(row_list->rows, (row_list->size + 1) * sizeof(row_t));
  memmove(row_list->rows + idx + 1, row_list->rows + idx, (row_list->size - idx) * sizeof(row_t));
  row_list->rows[idx] = row;
  row_list->size++;
}

/* Append a row to the row list */
void row_list_append(row_list_t* row_list, row_t row) {
  row_list_insert_at(row_list, row, row_list->size);
}

/* Delete a row from the list at a given index. */
void row_list_delete_at(row_list_t* row_list, size_t idx) {
  row_destroy(&row_list->rows[idx]);
  for (int i = idx; i < row_list->size - 1; i++) {
    row_list->rows[i] = row_list->rows[i + 1];
  }
  row_list->size--;
}

/* Free the memory held by a row list */
void row_list_destroy(row_list_t* row_list) {
  for (int i = 0; i < row_list->size; i++) {
    row_destroy(&row_list->rows[i]);
  }
  free(row_list->rows);
}