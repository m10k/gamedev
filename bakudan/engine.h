#ifndef ENGINE_H
#define ENGINE_H

#include "game.h"

int engine_init(void);
int engine_run(void);
int engine_quit(void);
void engine_set_state(game_state);

#endif /* ENGINE_H */
