typedef struct {
  char* build_command;
  char* test_command;
} config_t;

void config_init(config_t* config);

void config_destroy(config_t* config);

void config_parse(config_t* config);