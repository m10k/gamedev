#ifndef ANIM_H
#define ANIM_H

#include <SDL2/SDL.h>

typedef enum {
	ANIM_EXPLOSION,
	ANIM_NUM
} anim_type;

typedef struct {
	SDL_Surface *face;
	SDL_Rect rect;
} frame;

typedef struct {
	SDL_Surface *sprites;
	int nframes;
	int fpf;
	frame *frames;
} anim;

typedef struct _anim_inst anim_inst;

struct _anim_inst {
	anim *base;
	int frame;
	int fpf;
	int x;
	int y;
	anim_inst *next;
};

int anim_init(void);
int anim_quit(void);
int anim_draw(anim*, const int, const int, const int, SDL_Surface*);
int anim_inst_draw(anim_inst*, SDL_Surface*);
anim_inst* anim_get_inst(anim_type, const int, const int);

#endif /* ANIM_H */
