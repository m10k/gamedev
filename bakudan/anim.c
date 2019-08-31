#include <errno.h>
#include "anim.h"
#include "gfx.h"

static const char *_anim_paths[ANIM_NUM] = {
	"gfx/explosion.png",
	"gfx/abomb.png"
};

static frame explo_frames[] = {
	{
		NULL,
		{ 0, 0, 32, 32 }
	}, {
		NULL,
		{ 32, 0, 32, 32 }
	}, {
		NULL,
		{ 64, 0, 32, 32 }
	}, {
		NULL,
		{ 96, 0, 32, 32 }
	}, {
		NULL,
		{ 0, 32, 32, 32 }
	}, {
		NULL,
		{ 32, 32, 32, 32 }
	}, {
		NULL,
		{ 64, 32, 32, 32 }
	}, {
		NULL,
		{ 96, 32, 32, 32 }
	}, {
		NULL,
		{ 0, 64, 32, 32 }
	}, {
		NULL,
		{ 32, 64, 32, 32 }
	}
};

static frame abomb_frames[] = {
	{
		NULL, { 0, 0, 32, 32 }
	}, {
		NULL, { 32, 0, 32, 32 }
	}, {
		NULL, { 64, 0, 32, 32 }
	}, {
		NULL, { 96, 0, 32, 32 }
	}, {
		NULL, { 0, 32, 32, 32 }
	}, {
		NULL, { 32, 32, 32, 32 }
	}, {
		NULL, { 64, 32, 32, 32 }
	}, {
		NULL, { 96, 32, 32, 32 }
	}
};

static anim _anims[ANIM_NUM] = {
	{
		.sprites = NULL,
		.nframes = 10,
		.frames = &explo_frames[0],
		.fpf = 2 /* advance every 2 render frames */
	}, {
		.sprites = NULL,
		.nframes = 8,
		.frames = &abomb_frames[0],
		.fpf = 1
	}
};

int anim_init(void)
{
	int ret_val;
	int i;

	for(i = 0; i < ANIM_NUM; i++) {
		_anims[i].sprites = gfx_load_image(_anim_paths[i]);

		if(!_anims[i].sprites) {
			ret_val = -EFAULT;
			printf("\"%s\"を読み込めない\n", _anim_paths[i]);
		} else {
			int j;

			for(j = 0; j < _anims[i].nframes; j++) {
				_anims[i].frames[j].face = _anims[i].sprites;
			}
		}
	}

	if(ret_val < 0) {
		anim_quit();
	}

	return(ret_val);
}

int anim_quit(void)
{
	int ret_val;
	int i;

	ret_val = 0;

	for(i = 0; i < ANIM_NUM; i++) {
		if(_anims[i].sprites) {
			int j;

			SDL_FreeSurface(_anims[i].sprites);
			_anims[i].sprites = NULL;

			for(j = 0; j < _anims[i].nframes; j++) {
				_anims[i].frames[j].face = NULL;
			}
		}
	}

	return(ret_val);
}

int anim_draw(anim *a, const int frame,
			  const int x, const int y,
			  SDL_Surface *dst)
{
	int ret_val;

	ret_val = 0;

	if(frame < 0 || frame > a->nframes) {
		ret_val = -EINVAL;
	} else {
		SDL_Rect drect;

		drect.x = x * 32;
		drect.y = y * 32;
		drect.w = a->frames[frame].rect.w;
		drect.h = a->frames[frame].rect.h;

		SDL_BlitSurface(a->frames[frame].face, &(a->frames[frame].rect), dst, &drect);
	}

	return(ret_val);
}

int anim_inst_draw(anim_inst *a, SDL_Surface *dst)
{
	return(anim_draw(a->base, a->frame, a->x, a->y, dst));
}

anim_inst* anim_get_inst(anim_type type, const int x, const int y)
{
	anim_inst *a;

	if(type < 0 || type >= ANIM_NUM) {
		return(NULL);
	}

	a = malloc(sizeof(*a));

	if(a) {
		memset(a, 0, sizeof(*a));

		a->base = &(_anims[type]);
		a->frame = 0;
		a->x = x;
		a->y = y;
		a->fpf = a->base->fpf;
		a->cfpf = a->fpf;
	}

	return(a);
}
