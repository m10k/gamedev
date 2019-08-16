#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "gfx.h"
#include "game.h"
#include "anim.h"

#define FONT_PATH   "/usr/share/fonts/opentype/ipafont-gothic/ipag.ttf"
#define SFONT_SIZE  16
#define FONT_SIZE   32
#define PLAYER_CHAR "人"

#define STATS_WIDTH 256

#define H 0
#define V 1

static int _sdl_initialized;
static int _ttf_initialized;
static SDL_Window *_window;
static TTF_Font *_font;
static TTF_Font *_sfont;
static int _width = 32 * WIDTH + STATS_WIDTH;
static int _height = 32 * HEIGHT;
static SDL_Surface *_surface;
static SDL_Surface *_sprites[GAME_SPRITE_NUM];
static SDL_Color _textcolor = { 0x22, 0x22, 0x22 };
static int _menu_w = 0;
static int _menu_h = 0;
static int _menu_p[2] = { 16, 8 };
static SDL_Surface *_sboard;

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

static const char *_player_names[] = {
	"赤",
	"緑",
	"青",
	"黄"
};

static SDL_Surface *_menu_sprites[MENU_NUM];
static SDL_Surface *_env_sprites[SPRITE_NONE];
static SDL_Surface *_player_sprites[COLOR_NUM];

static int _gfx_generate(void);

void gfx_cleanup(void)
{
	if(_sboard) {
		SDL_FreeSurface(_sboard);
		_sboard = NULL;
	}

	return;
}

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

	_sfont = TTF_OpenFont(FONT_PATH, SFONT_SIZE);

	if(!_sfont) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		ret_val = -EFAULT;
		goto gtfo;
	}

	ret_val = _gfx_generate();

	if(ret_val < 0) {
		fprintf(stderr, "_gfx_generate: %s\n", strerror(-ret_val));
		goto gtfo;
	}

	ret_val = anim_init();

	if(ret_val < 0) {
		fprintf(stderr, "anim_init: %s\n", strerror(-ret_val));
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

		if(_sfont) {
			TTF_CloseFont(_sfont);
			_sfont = NULL;
		}

		TTF_Quit();
		_ttf_initialized = 0;
	}

	if(_sdl_initialized) {
		int i;

		anim_quit();

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

	return(0);
}

SDL_Surface *gfx_load_image(const char *path)
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
		_env_sprites[i] = gfx_load_image(_env_gfx[i]);

		if(!_env_sprites[i]) {
			ret_val = -EFAULT;
			fprintf(stderr, "gfx_load_image: %s\n", SDL_GetError());
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
	anim_inst *a;
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

				case OBJECT_TYPE_ITEM:
					/* draw item */

					switch(((item*)o)->type) {
					case ITEM_TYPE_BAG:
						st = SPRITE_BAG;
						break;

					case ITEM_TYPE_LIFE:
						st = SPRITE_LIFE;
						break;

					case ITEM_TYPE_LUCK:
						st = SPRITE_LUCK;
						break;

					case ITEM_TYPE_POTION:
						st = SPRITE_POTION;
						break;

					case ITEM_TYPE_POWER:
						st = SPRITE_POWER;
						break;

					case ITEM_TYPE_TIME:
						st = SPRITE_TIME;
						break;

					default:
						printf("BUG [%s:%d] Unknown item type\n", __FILE__, __LINE__);
						break;
					}

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

	/* draw animations */
	for(a = game_get_anims(); a; a = a->next) {
		anim_inst_draw(a, _surface);
	}

	return(ret_val);
}

void gfx_draw_stats(void)
{
	static SDL_Surface *header;
	static SDL_Surface *header2;
	SDL_Rect drect;
	int i;

	if(!header) {
		header = TTF_RenderUTF8_Solid(_sfont,
									  "   殺   死   自   岩   物",
									  _textcolor);
	}

	if(!header2) {
		header2 = TTF_RenderUTF8_Solid(_sfont,
									   "   HP   弾   運   力   時   命",
									   _textcolor);
	}

	drect.x = 32 * WIDTH + 8;
	drect.y = 8;

	if(header) {
		SDL_BlitSurface(header, NULL, _surface, &drect);
		drect.y += header->h + 4;

		for(i = 0; i < game_num_players(); i++) {
			player *p;
			SDL_Surface *s;
			char line[128];

			p = game_player_num(i);

			snprintf(line, sizeof(line), " %4d %4d %4d %4d %4d",
					 p->frags, p->deaths, p->suicides, p->boulders, p->items);

			s = TTF_RenderUTF8_Solid(_sfont, line, _player_color[i]);

			if(s) {
				SDL_BlitSurface(s, NULL, _surface, &drect);
				drect.y += header->h + 4;
				SDL_FreeSurface(s);
			}
		}
	}

	drect.y += 16;

	if(header2) {
		SDL_BlitSurface(header2, NULL, _surface, &drect);
		drect.y += header2->h + 4;

		for(i = 0; i < game_num_players(); i++) {
			player *p;
			SDL_Surface *s;
			char line[128];

			p = game_player_num(i);

			if(p->health < 0) {
				snprintf(line, sizeof(line), "%4d %4d %4d %4d %4d %4d",
						 p->health, p->bombs, p->probability,
						 p->bomb_strength, p->bomb_timeout, p->lifes);
			} else {
				snprintf(line, sizeof(line), " %4d %4d %4d %4d %4d %4d",
						 p->health, p->bombs, p->probability,
						 p->bomb_strength, p->bomb_timeout, p->lifes);
			}

			s = TTF_RenderUTF8_Solid(_sfont, line, _player_color[i]);

			if(s) {
				SDL_BlitSurface(s, NULL, _surface, &drect);
				drect.y += header2->h + 4;
				SDL_FreeSurface(s);
			}
		}
	}

	return;
}

void gfx_update_window(void)
{
	SDL_UpdateWindowSurface(_window);
	return;
}

void gfx_draw_winner(void)
{
	int winner;
	SDL_Surface *s;
	char str[128];

	winner = game_get_winner();

	snprintf(str, sizeof(str), "%sの勝ちだ！＼（＾＿＾）／", _player_names[winner]);

	s = TTF_RenderUTF8_Solid(_font, str, _player_color[winner]);

	if(s) {
		SDL_Rect drect;

		drect.w = s->w + 10;
		drect.h = s->h + 10;
		drect.x = (_surface->w - drect.w) / 2;
		drect.y = (_surface->h - drect.h) / 2;

		SDL_FillRect(_surface, &drect, SDL_MapRGB(_surface->format,
												  0, 0, 0));

		drect.x++;
		drect.y++;
		drect.w -= 2;
		drect.h -= 2;

		SDL_FillRect(_surface, &drect, SDL_MapRGB(_surface->format,
												  0xff, 0xff, 0xff));

		drect.x += 4;
		drect.y += 4;
		drect.w = s->w;
		drect.h = s->h;

		SDL_BlitSurface(s, NULL, _surface, &drect);

		SDL_FreeSurface(s);
	}

	return;
}
