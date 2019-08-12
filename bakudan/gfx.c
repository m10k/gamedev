#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "gfx.h"
#include "game.h"

#define FONT_PATH "/usr/share/fonts/opentype/ipafont-gothic/ipag.ttf"
#define FONT_SIZE 32
#define PLAYER_CHAR "人"

#define H 0
#define V 1

static int _sdl_initialized;
static int _ttf_initialized;
static SDL_Window *_window;
static TTF_Font *_font;
static int _width = 32 * WIDTH;
static int _height = 32 * HEIGHT;
static SDL_Surface *_surface;
static SDL_Surface *_sprites[GAME_SPRITE_NUM];
static SDL_Color _textcolor = { 0x22, 0x22, 0x22 };
static int _menu_w = 0;
static int _menu_h = 0;
static int _menu_p[2] = { 16, 8 };

static SDL_Color _env_color[] = {
	{ 0x22, 0x22, 0x22 },
	{ 0x44, 0x44, 0x44 },
	{ 0x88, 0x44, 0x44 }
};

static SDL_Color _player_color[] = {
	{ 0xff, 0x00, 0x00 },
	{ 0x00, 0xff, 0x00 },
	{ 0x00, 0x00, 0xff },
	{ 0x00, 0xff, 0xff }
};

enum {
	MENU_SP = 0,
	MENU_MP,
	MENU_QUIT,
	MENU_NUM
};

static const char *_menu_caption[MENU_NUM] = {
	"One player",
	"Two players",
	"Exit"
};

static const char *_env_gfx[SPRITE_NONE] = {
	"gfx/wall.png",
	"gfx/pillar.png",
	"gfx/boulder.png",
	"gfx/bomb.png",
	"gfx/bag.png",
	"gfx/life.png",
	"gfx/luck.png",
	"gfx/potion.png",
	"gfx/power.png",
	"gfx/time.png"
};

static const char *_env_caption[SPRITE_NONE] = {
	"壁",
	"柱",
	"岩",
	"弾"
};

static SDL_Surface *_menu_sprites[MENU_NUM];
static SDL_Surface *_env_sprites[SPRITE_NONE];
static SDL_Surface *_player_sprites[COLOR_NUM];

static int _gfx_generate(void);

int gfx_init(void)
{
	int ret_val;

	ret_val = 0;

	if(SDL_Init(SDL_INIT_VIDEO)) {
		ret_val = -EFAULT;
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		goto gtfo;
	}

	_sdl_initialized = 1;

	if(TTF_Init()) {
		ret_val = -EFAULT;
		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
		goto gtfo;
	}

	_ttf_initialized = 1;

	_window = SDL_CreateWindow("bakudan",
							   SDL_WINDOWPOS_UNDEFINED,
							   SDL_WINDOWPOS_UNDEFINED,
							   _width, _height,
							   SDL_WINDOW_SHOWN);

	if(!_window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		ret_val = -EFAULT;
		goto gtfo;
	}

	_surface = SDL_GetWindowSurface(_window);
	assert(_surface);

	SDL_FillRect(_surface, NULL, SDL_MapRGB(_surface->format, 0xff, 0xff, 0xff));
	SDL_UpdateWindowSurface(_window);

	_font = TTF_OpenFont(FONT_PATH, FONT_SIZE);

	if(!_font) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		ret_val = -EFAULT;
		goto gtfo;
	}

	ret_val = _gfx_generate();

	if(ret_val < 0) {
		fprintf(stderr, "_gfx_generate: %s\n", strerror(-ret_val));
		goto gtfo;
	}

gtfo:
	if(ret_val < 0) {
		gfx_quit();
	}

	return(ret_val);
}

int gfx_quit(void)
{
	if(_ttf_initialized) {
		if(_font) {
			TTF_CloseFont(_font);
			_font = NULL;
		}

		TTF_Quit();
		_ttf_initialized = 0;
	}

	if(_sdl_initialized) {
		int i;

		for(i = 0; i < MENU_NUM; i++) {
			if(_menu_sprites[i]) {
				SDL_FreeSurface(_menu_sprites[i]);
				_menu_sprites[i] = NULL;
			}
		}

		for(i = 0; i < SPRITE_NONE; i++) {
			if(_env_sprites[i]) {
				SDL_FreeSurface(_env_sprites[i]);
				_env_sprites[i] = NULL;
			}
		}

		for(i = 0; i < COLOR_NUM; i++) {
			if(_player_sprites[i]) {
				SDL_FreeSurface(_player_sprites[i]);
				_player_sprites[i] = NULL;
			}
		}

		if(_window) {
			SDL_DestroyWindow(_window);
		}

		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		_sdl_initialized = 0;
	}

	return(-ENOSYS);
}

static SDL_Surface *_load_image(const char *path)
{
	SDL_Surface *raw;
	SDL_Surface *ret_val;

	raw = IMG_Load(path);
	ret_val = NULL;

	if(raw) {
		ret_val = SDL_ConvertSurface(raw, _surface->format, 0);

		if(ret_val) {
			SDL_FreeSurface(raw);
		} else {
			ret_val = raw;
		}

		SDL_SetColorKey(ret_val, SDL_TRUE,
						SDL_MapRGB(ret_val->format,
								   0xff, 0x00, 0xff));
	}

	return(ret_val);
}

static int _gfx_generate(void)
{
	int ret_val;
	int i;

	memset(_menu_sprites, 0, sizeof(_menu_sprites));
	memset(_env_sprites, 0, sizeof(_env_sprites));

	for(ret_val = 0, i = 0; i < MENU_NUM; i++) {
		_menu_sprites[i] = TTF_RenderUTF8_Solid(_font, _menu_caption[i], _textcolor);

		if(!_menu_sprites[i]) {
			ret_val = -EFAULT;
			fprintf(stderr, "TTF_RenderUTF8_Solid: %s\n", TTF_GetError());
			goto cleanup;
		}

		if(_menu_sprites[i]->w > _menu_w) {
			_menu_w = _menu_sprites[i]->w;
		}
		_menu_h += _menu_sprites[i]->h + _menu_p[V] + _menu_p[V];
	}
	_menu_w += _menu_p[H] + _menu_p[H];

	for(i = 0; i < SPRITE_NONE; i++) {
		_env_sprites[i] = _load_image(_env_gfx[i]);

		if(!_env_sprites[i]) {
			ret_val = -EFAULT;
			fprintf(stderr, "_load_image: %s\n", SDL_GetError());
			goto cleanup;
		}
	}

	for(i = 0; i < COLOR_NUM; i++) {
		_player_sprites[i] = TTF_RenderUTF8_Solid(_font, PLAYER_CHAR, _player_color[i]);

		if(!_player_sprites[i]) {
			ret_val = -EFAULT;
			fprintf(stderr, "TTF_RenderUTF8_Solid: %s\n", TTF_GetError());
			goto cleanup;
		}
	}

cleanup:
	if(ret_val < 0) {
		for(i = 0; i < MENU_NUM; i++) {
			if(_menu_sprites[i]) {
				SDL_FreeSurface(_menu_sprites[i]);
				_menu_sprites[i] = NULL;
			}
		}

		for(i = 0; i < SPRITE_NONE; i++) {
			if(_env_sprites[i]) {
				SDL_FreeSurface(_env_sprites[i]);
				_env_sprites[i] = NULL;
			}
		}

		for(i = 0; i < COLOR_NUM; i++) {
			SDL_FreeSurface(_player_sprites[i]);
			_player_sprites[i] = NULL;
		}
	}

	return(ret_val);
}

int gfx_draw_menu(int selection)
{
	int ret_val;
    int i;

	/* fill with white */
	SDL_FillRect(_surface, NULL, SDL_MapRGB(_surface->format, 0xff, 0xff, 0xff));

	for(i = 0; i < MENU_NUM; i++) {
		SDL_Rect dst;

		dst.x = (_surface->w - _menu_sprites[i]->w) >> 1;
		dst.y = ((_surface->h - _menu_h) >> 1) +
			i * _menu_sprites[i]->h + _menu_p[V] +
			i * 2 * _menu_p[V];

		if(selection == i) {
			SDL_Rect brdr;

			brdr.x = ((_surface->w - _menu_w) >> 1) - 1;
			brdr.y = dst.y - 1;
			brdr.w = _menu_w + 2;
			brdr.h = _menu_sprites[i]->h + 2;

			SDL_FillRect(_surface, &brdr, SDL_MapRGB(_surface->format, 0x0, 0x0, 0x0));

			brdr.x++;
			brdr.y++;
			brdr.w -= 2;
			brdr.h -= 2;

			SDL_FillRect(_surface, &brdr, SDL_MapRGB(_surface->format, 0xff, 0xff, 0xff));
		}

		SDL_BlitSurface(_menu_sprites[i], NULL, _surface, &dst);
	}

	SDL_UpdateWindowSurface(_window);

	return(ret_val);
}

int gfx_draw_game(void)
{
	int ret_val;
	int x, y;

	ret_val = 0;

	/* fill with white */
	SDL_FillRect(_surface, NULL, SDL_MapRGB(_surface->format, 0xff, 0xff, 0xff));

	for(x = 0; x <= WIDTH; x++) {
		for(y = 0; y <= HEIGHT; y++) {
			object *o;
			sprite_type st;

			o = game_object_at(x, y);
			st = SPRITE_NONE;

			if(o) {
				switch(o->type) {
				case OBJECT_TYPE_WALL:
					/* draw wall */
					st = SPRITE_WALL;
					break;

				case OBJECT_TYPE_PILLAR:
					/* draw pillar */
					st = SPRITE_PILLAR;
					break;

				case OBJECT_TYPE_BOULDER:
					/* draw boulder */
					st = SPRITE_BOULDER;
					break;

				case OBJECT_TYPE_BOMB:
					/* draw bomb */
					st = SPRITE_BOMB;
					break;

				default:
					break;
				}
			}

			if(st < SPRITE_NONE) {
				SDL_Rect drect;

				drect.x = x * 32;
				drect.y = y * 32;
				drect.w = 32;
				drect.h = 32;

				SDL_BlitSurface(_env_sprites[st], NULL, _surface, &drect);
			}
		}
	}

	for(x = 0; x < game_num_players(); x++) {
		player *p;
		SDL_Rect dpos;

		p = game_player_num(x);

		if(p->health <= 0) {
			continue;
		}

		dpos.x = (obj_x(p) * 32) + p->dx;
		dpos.y = (obj_y(p) * 32) + p->dy;

		SDL_BlitSurface(_player_sprites[p->num], NULL, _surface, &dpos);
	}

	SDL_UpdateWindowSurface(_window);

	return(ret_val);
}
