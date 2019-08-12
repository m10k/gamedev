#ifndef GAME_H
#define GAME_H

#define WIDTH 17
#define HEIGHT 17

#define FPS             50
#define TICKS_PER_FRAME (1000 / FPS)

/*
  #########
  #  ...  #
  # #.#.# #
  #.......#
  # #.#.# #
  #  ...  #
  #########

  Spawn  : (x < 2 && y < 2) || (x > WIDTH - 1) && (y > HEIGHT - 1) ||
           (x < 2) && (y > HEIGHT - 1) && (y < 2) && (x > WIDTH - 1)
  Wall   : x == 0 || x == WIDTH || y == 0 || y == HEIGHT
  Pillar : x % 2 == 0 && y % 2 == 0 && (!wall)
  Boulder: !wall && !pillar && !spawn
*/

#define PLAYER_DEFAULT_STRENGTH 1000
#define PLAYER_DEFAULT_TIMEOUT  5
#define PLAYER_DEFAULT_HEALTH   500

#define BOMB_GRADIENT 500

enum {
	COLOR_RED = 0,
	COLOR_GREEN,
	COLOR_BLUE,
	COLOR_YELLOW,
	COLOR_NUM
};

typedef enum {
	GAME_STATE_MENU,
	GAME_STATE_SP,
	GAME_STATE_MP,
	GAME_STATE_PAUSE,
	GAME_STATE_SBOARD
} game_state;

typedef enum {
	GAME_SPRITE_MENU_SP = 0,
	GAME_SPRITE_MENU_MP,
	GAME_SPRITE_MENU_QUIT,
	GAME_SPRITE_NUM
} game_sprite;

typedef enum {
	OBJECT_TYPE_WALL,
	OBJECT_TYPE_PILLAR,
	OBJECT_TYPE_BOULDER,
	OBJECT_TYPE_ITEM,
	OBJECT_TYPE_BOMB
} object_type;

typedef struct {
	object_type type;
	int x;
	int y;
	int passable;
} object;

typedef enum {
	PLAYER_HUMAN,
	PLAYER_CPU
} player_type;

typedef struct {
	object __parent;

	player_type type;
	int num;
	int dx;
	int dy;
	int spawn_x;
	int spawn_y;
	int health;
	int bomb_timeout;
	int bomb_strength;
	int lifes;
} player;

typedef struct {
	object __parent;

	int strength;
} boulder;

typedef struct {
	object __parent;

	int timeout;
	int strength;
} bomb;

#define obj_x(o) (((object*)o)->x)
#define obj_y(o) (((object*)o)->y)

int game_init(const int, const int);
object* game_object_at(const int, const int);
player* game_player_num(const int);
int game_num_players(void);
void game_animate(void);
void game_logic(void);

void game_player_move(const int, const int, const int);
void game_player_action(const int);

#endif /* GAME_H */
