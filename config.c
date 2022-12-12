#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void config_init(config_t* config) {
  config->build_command = NULL;
  config->test_command = NULL;
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
    if (config_file == NULL) {
      // If still not found, then do nothing
      return;
    } else {
      // Parse the found config file
    }
  }
}