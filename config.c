#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void config_init(config_t* config) {
  config->build_command = NULL;
  config->test_command = NULL;
}

void config_destroy(config_t* config) {
  free(config->build_command);
  free(config->test_command);
}

static void trim_whitespace(char* str) {
  // Find the first non-whitespace character
  int start = 0;
  while (isspace(str[start])) 
    start++;

  // Find the last non-whitespace character
  int end = strlen(str) - 1;
  while (end >= 0 && isspace(str[end]))
    end--;

  // Shift the string so that it starts and ends at the first/last non-whitespace character
  int length = end - start + 1;
  memmove(str, str + start, length);
  str[length] = '\0';
}

void config_parse(config_t* config) {
  const char* config_name = ".buffalorc";

  // First, look in the current directory
  FILE* config_file = fopen(config_name, "r");
  if (config_file == NULL) {
    // If not found, look in the home directory
    char home_config[strlen(config_name) + 3];
    strcpy(home_config, "~/");
    strcat(home_config, config_name);
    
    config_file = fopen(home_config, "r");
  }

  // If config was found now
  if (config_file != NULL) {
    // Parse the found config file
    if (fseek(config_file, 0, SEEK_END) != 0) {
      perror("Unable to seek to end of file");
      exit(2);
    }

    // Get the size of the file
    size_t size = ftell(config_file);

    // Seek back to the beginning of the file
    if (fseek(config_file, 0, SEEK_SET) != 0) {
      perror("Unable to seek to beginning of file");
      exit(2);
    }

    // Null terminated buffer to hold file contents
    char* data = malloc(size + 1);

    // Read the file contents
    if (fread(data, sizeof(char), size, config_file) != size) {
      fprintf(stderr, "Failed to read entire file\n");
      exit(2);
    }

    // Null terminate data
    data[size] = '\0';

    // Parse file line by line
    char* line;
    char* data_temp = data; // strsep mangles this pointer, so save it
    while ((line = strsep(&data_temp, "\n")) != NULL) {
      if (strcmp(line, "") != 0) { // skip blank lines
        char* key = strsep(&line, ":");
        if (key == NULL || line == NULL) {
          fprintf(stderr, "Failed to parse config file: missing colon.\n");
          exit(1);
        }

        // Value is the remaining portion of the line
        char* value = line;
        
        // Trim whitespace from key and value
        trim_whitespace(key);
        trim_whitespace(value);

        if (strcmp(key, "build") == 0) {
          config->build_command = strdup(value);
        } else if (strcmp(key, "test") == 0) {
          config->test_command = strdup(value);
        } else {
          fprintf(stderr, "Failed to parse config file: unknown key.\n");
          exit(1);
        }
      }
    }

    free(data);
  }
}