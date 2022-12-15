#pragma once

#include "buffalo_state.h"

void ui_init(buffalo_state_t* bs);

void* ui_run(void* arg);

void ui_exit(buffalo_state_t* bs);
