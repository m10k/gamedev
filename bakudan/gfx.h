#ifndef GFX_H
#define GFX_H

typedef enum {
	SPRITE_WALL = 0,
	SPRITE_PILLAR,
	SPRITE_BOULDER,
	SPRITE_BOMB,
	SPRITE_NONE
} sprite_type;

int gfx_init(void);
int gfx_quit(void);

int gfx_draw_menu(int);
int gfx_draw_game(void);

#endif /* GFX_H */
