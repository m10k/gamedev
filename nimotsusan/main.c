#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "level.h"

#define WIDTH  8
#define HEIGHT 8
#define NOBJS  2
#define FONT_SIZE 32

static int stop = 0;
static struct level *_lvl = NULL;

static const char *obj_text[] = {
	"　",
	"箱",
	"口",
	"人",
	"◯",
	"壁",
	"無"
};

static int dx;
static int dy;

static int _sdl_initialized;
static int _ttf_initialized;
static SDL_Window *_window;
static SDL_Surface *_surface;
static TTF_Font *_font;
static SDL_Surface *_sprites[OBJ_UNKNOWN];
static SDL_Color _textcolor = {0x22, 0x22, 0x22};
static int _width = 32 * WIDTH;
static int _height = 32 * HEIGHT;

static int generate_gfx(void)
{
	int ret_val;
	int i;

	for(ret_val = 0, i = 0; i < OBJ_UNKNOWN; i++) {
		_sprites[i] = TTF_RenderUTF8_Solid(_font, obj_text[i], _textcolor);

		if(!_sprites[i]) {
			ret_val = -EFAULT;
			fprintf(stderr, "TTF_RenderUTF8_Solid: %s\n", TTF_GetError());
			break;
		}
//		SDL_FillRect(_sprites[i], NULL, SDL_MapRGB(_sprites[i]->format, 0xff, 0x00, 0x00));
	}

	if(ret_val < 0) {
		while(i >= 0) {
			SDL_FreeSurface(_sprites[i]);
			_sprites[i] = NULL;
			i--;
		}
	}

	return(ret_val);
}

void uninit(void)
{
	int i;

	if(_window) {
		SDL_DestroyWindow(_window);
		_window = NULL;
	}

	_surface = NULL;

	if(_font) {
		TTF_CloseFont(_font);
		_font = NULL;
	}

	for(i = 0; i < OBJ_UNKNOWN; i++) {
		if(_sprites[i]) {
			SDL_FreeSurface(_sprites[i]);
			_sprites[i] = NULL;
		}
	}

	if(_ttf_initialized) {
		TTF_Quit();
		_ttf_initialized = 0;
	}

	if(_sdl_initialized) {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		_sdl_initialized = 0;
	}

	return;
}

static int init(void)
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

	_window = SDL_CreateWindow("nimotsusan",
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

	_font = TTF_OpenFont("/usr/share/fonts/opentype/ipafont-gothic/ipag.ttf", FONT_SIZE);

	if(!_font) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		ret_val = -EFAULT;
		goto gtfo;
	}

	ret_val = generate_gfx();

	if(ret_val < 0) {
		fprintf(stderr, "Could not generate graphics\n");
		goto gtfo;
	}

	ret_val = readlvl(&_lvl, "foo.lvl");

	if(ret_val < 0) {
		fprintf(stderr, "readlvl: %s\n", strerror(-ret_val));
		goto gtfo;
	}

gtfo:
	if(ret_val < 0) {
		uninit();
	}

	return(ret_val);
}

static void input(void)
{
	SDL_Event ev;

	dx = 0;
	dy = 0;

	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
		case SDL_QUIT:
			stop = 1;
			return;

		case SDL_KEYUP:
			switch(ev.key.keysym.sym) {
			case SDLK_q:
				stop = 1;
				return;

			case SDLK_w:
				dy = -1;
				break;

			case SDLK_a:
				dx = -1;
				break;

			case SDLK_s:
				dy = 1;
				break;

			case SDLK_d:
				dx = 1;
				break;

			default:
				break;
			}
			/* don't read more events */
			return;

		default:
			/* ignore */
			break;
		}
	}

	return;
}

struct object* object_at(int x, int y)
{
	struct object *cur;

	for(cur = _lvl->objects; cur; cur = cur->next) {
		if(cur->x == x && cur->y == y) {
			return(cur);
		}
	}

	return(NULL);
}

static int unpassable(int x, int y)
{
	struct object *o;

	if(x < 0 || y < 0 || x >= _lvl->width || y >= _lvl->height) {
		return(1);
	}

	o = object_at(x, y);

	if(!o || o->type == OBJ_KUCHI) {
		return(0);
	}

	return(1);
}

static void move_object(struct object *o, int x, int y)
{
	o->x = x;
	o->y = y;

	return;
}

static int done(void)
{
	struct object *cur;

	for(cur = _lvl->objects; cur; cur = cur->next) {
		if(cur->type == OBJ_HAKO) {
			return(0);
		}
	}

	return(1);
}

static void process(void)
{
	int px, py;
	int rem;

	px = _lvl->player->x + dx;
	py = _lvl->player->y + dy;

	if(unpassable(px, py)) {
		int nx, ny;
		struct object *ni;

		nx = px + dx;
		ny = py + dy;
		ni = object_at(px, py);

		/* check if occupator is not a wall and if it can be moved */
		if(ni->type != OBJ_KABE && !unpassable(nx, ny)) {
			struct object *d;

			d = object_at(nx, ny);

			move_object(ni, nx, ny);
			move_object(_lvl->player, px, py);

			/* passable but occupied means the nimotsu is on the destination */
			if(d) {
				d->type = OBJ_MARU;
			}
		}
	} else {
		move_object(_lvl->player, px, py);
	}

	if(done()) {
		stop = 1;
	}

	return;
}

static void draw_at(SDL_Surface *dst, int x, int y, obj_type type)
{
	SDL_Surface *src;
	SDL_Rect drect;

	src = _sprites[type];

	if(src) {
		drect.x = x * 32;
		drect.y = y * 32;
		drect.w = 32;
		drect.h = 32;

		SDL_BlitSurface(src, NULL, dst, &drect);
	}

	return;
}

static void output(void)
{
	struct object *obj;
	int x, y;

	/* fill with background color */
	SDL_FillRect(_surface, NULL, SDL_MapRGB(_surface->format, 0xff, 0xff, 0xff));

	for(obj = _lvl->objects; obj; obj = obj->next) {
		draw_at(_surface, obj->x, obj->y, obj->type);
	}

	SDL_UpdateWindowSurface(_window);

	return;
}

int main(int argc, char *argv[])
{
	int ret_val;

	ret_val = init();

	if(ret_val < 0) {
		fprintf(stderr, "Initialization failed\n");
	} else {
		while(!stop) {
			input();
			process();
			output();
		}
	}

	return(0);
}
