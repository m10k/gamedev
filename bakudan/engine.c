#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "engine.h"
#include "gfx.h"
#include "game.h"

static int _stop;
static game_state _state;
static int _menu_selection;

int engine_init(void)
{
	int ret_val;

	ret_val = gfx_init();

	if(ret_val < 0) {
		fprintf(stderr, "gfx_init: %s\n", strerror(-ret_val));
	} else {
		/* perform remaining initialization */
		_stop = 0;
		_state = GAME_STATE_MENU;
		_menu_selection = 0;

		if(ret_val < 0) {
			fprintf(stderr, "game_init: %s\n", strerror(-ret_val));
		}
	}

	return(ret_val);
}

static void _menu_execute(int sel)
{
	switch(sel) {
	case 0:
		printf("1Pゲーム");
		_state = GAME_STATE_SP;
		game_init(1, 1);
		break;

	case 1:
		printf("2Pゲーム");
		_state = GAME_STATE_MP;
		break;

	case 2:
		printf("Exit");
		_stop = 1;
		break;
	}

	printf("を選んだ\n");

	if(_state == GAME_STATE_MP) {
		printf("2Pゲームは実装されていない\n");
		_state = GAME_STATE_MENU;
	}

	return;
}

static void _menu_handle_input(SDL_Event *ev)
{
	switch(ev->key.keysym.sym) {
	case SDLK_q:
		_stop = 1;
		return;

	case SDLK_w:
		if(--_menu_selection < 0) {
			_menu_selection = 2;
		}

		break;

	case SDLK_a:
		break;

	case SDLK_s:
		if(++_menu_selection > 2) {
			_menu_selection = 0;
		}
		break;

	case SDLK_d:
		_menu_execute(_menu_selection);
		break;

	default:
		break;
	}

	return;
}

static void _game_handle_input_sp(SDL_Event *ev)
{
	switch(ev->key.keysym.sym) {
	case SDLK_q:
		_stop = 1;
		break;

	case SDLK_w:
		/* move up */
		game_player_move(0, 0, -1);
		break;

	case SDLK_a:
		/* move left */
		game_player_move(0, -1, 0);
		break;

	case SDLK_s:
		/* move down */
		game_player_move(0, 0, 1);
		break;

	case SDLK_d:
		/* move right */
		game_player_move(0, 1, 0);
		break;

	case SDLK_e:
		/* plant bomb */
		game_player_action(0);
		break;

	default:
		break;
	}

	return;
}

static void _input(void)
{
	SDL_Event ev;

	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
		case SDL_QUIT:
			_stop = 1;
			return;

		case SDL_KEYUP:
			if(_state == GAME_STATE_MENU) {
				_menu_handle_input(&ev);
			} else if(_state == GAME_STATE_SP) {
				_game_handle_input_sp(&ev);
			}

			break;

		default:
			break;
		}
	}

	return;
}

static void _process(void)
{
	if(_state != GAME_STATE_SP &&
	   _state != GAME_STATE_MP) {
		return;
	}

	game_logic();
	game_animate();

	return;
}

static void _output(void)
{
	switch(_state) {
	case GAME_STATE_MENU:
		/* draw menu */
		gfx_draw_menu(_menu_selection);
		break;

	case GAME_STATE_SP:
		gfx_draw_game();
		break;

	case GAME_STATE_MP:
	case GAME_STATE_PAUSE:
	case GAME_STATE_SBOARD:
		/* draw game state */

		if(_state == GAME_STATE_PAUSE) {
			/* draw pause overlay */
		} else if(_state == GAME_STATE_SBOARD) {
			/* draw scoreboard */
		}
		break;

	default:
		assert(0);
		break;
	}

	return;
}

int engine_run(void)
{
	unsigned elapsed;

	while(!_stop) {
		elapsed = SDL_GetTicks();

		_input();
		_process();
		_output();

		elapsed = SDL_GetTicks() - elapsed;

		if(elapsed < TICKS_PER_FRAME) {
			SDL_Delay(TICKS_PER_FRAME - elapsed);
		}
	}

	return(0);
}

int engine_quit(void)
{
	int ret_val;

	ret_val = gfx_quit();

	if(ret_val < 0) {
		fprintf(stderr, "gfx_quit: %s\n", strerror(-ret_val));
	}

	/* perform remaining cleanup */

	return(ret_val);
}

void engine_set_state(game_state state)
{
	_state = state;
	return;
}
