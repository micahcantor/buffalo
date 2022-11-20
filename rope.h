#ifndef ROPE_H
#define ROPE_H

#include <string.h>

#define MAX_LEAF_LEN 8

typedef struct rope_node {
  char data[MAX_LEAF_LEN];
  size_t weight; // length of all nodes in left subtree
  size_t length;
  struct rope_node* left;
  struct rope_node* right;
} rope_t;

typedef struct {
  rope_t* left;
  rope_t* right;
} rope_pair_t;

void rope_init(rope_t* rope);

void rope_destroy(rope_t* rope);

void rope_from_string(rope_t* rope, char* str, size_t n);

char rope_index_of(rope_t* rope, size_t idx);

rope_t* rope_concat(rope_t* left, rope_t* right);

rope_pair_t rope_split(rope_t* rope, size_t idx);

void rope_print(rope_t* rope);

#endif