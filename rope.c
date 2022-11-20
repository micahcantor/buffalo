#include "rope.h"
#include <stdio.h>
#include <stdlib.h>

void rope_init(rope_t* rope) {
  rope->weight = 0;
  rope->length = 0;
  rope->left = NULL;
  rope->right = NULL;
} 

void rope_destroy(rope_t* rope) {
  if (rope->left != NULL) {
    rope_destroy(rope->left);
    free(rope->left);
  }
  if (rope->right != NULL) {
    rope_destroy(rope->right);
    free(rope->right);
  }
}

void rope_from_string(rope_t* rope, char* str, size_t n) {
  if (n <= MAX_LEAF_LEN) {
    strncpy(rope->data, str, n);
    rope->length = n;
    rope->weight = n;
    rope->left = NULL;
    rope->right = NULL;
  } else {
    size_t m = n / 2;
    rope->weight += m;
    rope->left = malloc(sizeof(rope_t));
    rope->right = malloc(sizeof(rope_t));
    rope_init(rope->left);
    rope_init(rope->right);
    rope_from_string(rope->left, str, m);
    rope_from_string(rope->right, str + m, n - m);
  }
}

char rope_index_of(rope_t* rope, size_t idx) {
  if (rope->left == NULL && rope->right == NULL) {
    return rope->data[idx];
  } else if (idx < rope->weight) {
    return rope_index_of(rope->left, idx);
  } else {
    return rope_index_of(rope->right, idx - rope->weight);
  }
}

rope_t* rope_concat(rope_t* left, rope_t* right) {
  rope_t* parent = malloc(sizeof(rope_t));
  parent->length = 0;
  parent->weight = left->weight;
  parent->left = left;
  parent->right = right;
  return parent;
}

rope_pair_t rope_split(rope_t* rope, size_t idx) {
  if (idx < rope->weight) {
    rope_pair_t split = rope_split(rope->left, idx);
    rope_pair_t result;
    result.left = split.left;
    result.right = rope_concat(split.right, rope->right);
    return result;
  } else if (idx > rope->weight) {
    rope_pair_t split = rope_split(rope->right, idx - rope->weight);
    rope_pair_t result;
    result.left = rope_concat(rope->left, split.left);
    result.right = split.right;
    return result;
  } else {
    rope_pair_t result;
    result.left = rope->left;
    result.right = rope->right;
    return result;
  }
}

static void rope_print_helper(rope_t* rope) {
  if (rope == NULL) {
    return;
  } else if (rope->left == NULL && rope->right == NULL) {
    for (size_t i = 0; i < rope->length; i++) {
      putchar(rope->data[i]);
    }
  }
  rope_print_helper(rope->left);
  rope_print_helper(rope->right);
}

void rope_print(rope_t* rope) {
  rope_print_helper(rope);
  putchar('\n');
}