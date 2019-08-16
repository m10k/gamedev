#ifndef GFX_H
#define GFX_H

typedef enum {
	SPRITE_WALL = 0,
	SPRITE_PILLAR,
	SPRITE_BOULDER,
	SPRITE_BOMB,
	SPRITE_BAG,
	SPRITE_LIFE,
	SPRITE_LUCK,
	SPRITE_POTION,
	SPRITE_POWER,
	SPRITE_TIME,
	SPRITE_NONE
} sprite_type;

int gfx_init(void);
int gfx_quit(void);

SDL_Surface* gfx_load_image(const char*);
int gfx_draw_menu(int);
int gfx_draw_game(void);
void gfx_draw_winner(void);
void gfx_draw_stats(void);
void gfx_update_window(void);
void gfx_cleanup(void);

#endif /* GFX_H */
