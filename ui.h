#pragma once

#include "buffalo_state.h"

/**
 * Initialize the user interface and set up a callback function that should be
 * called every time there is a new message to send.
 *
 * \param callback  A function that should run every time there is new input.
 *                  The string passed to the callback should be copied if you
 *                  need to retain it after the callback function returns.
 */
void ui_init(buffalo_state_t* bs);

/**
 * Run the main UI loop. This function will only return the UI is exiting.
 */
void ui_run(buffalo_state_t* bs);

/**
 * Stop the user interface and clean up.
 */
void ui_exit(buffalo_state_t* bs);
