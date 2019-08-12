#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "game.h"

static player *players = NULL;
static player *cpu = NULL;
static int nplayers = 0;
static object *objects[WIDTH][HEIGHT];

#define IS_WALL(x,y)    (x == 0 || y == 0 || x == (WIDTH - 1) || y == (HEIGHT - 1))
#define IS_PILLAR(x,y)  (x > 0 && y > 0 && (x % 2 == 0) && (y % 2 == 0))
#define IS_SPAWN(x,y)   (!IS_WALL(x,y) && ((x <= 2 && y <= 2) || \
										   ((x >= WIDTH - 3) && (y >= HEIGHT - 3)) || \
										   (x <= 2 && (y >= HEIGHT - 3)) || \
										   (x >= WIDTH - 3) && (y <= 2)))
#define IS_BOULDER(x,y) (!IS_WALL(x,y) && !IS_PILLAR(x,y) && !IS_SPAWN(x,y))

#define PLX(n) ((object*)(&(players[n])))->x
#define PLY(n) ((object*)(&(players[n])))->y

#define SETPSPAWN(n,x,y) do {	  \
		players[n].spawn_x = (x); \
		players[n].spawn_y = (y); \
		SETPPOS(n,x,y);			  \
	} while(0)

#define SETPPOS(n,x,y) do {						\
		PLX(n) = (x);							\
		PLY(n) = (y);							\
	} while(0)

static object* make_object(object_type type, int x, int y)
{
	object *o;
	size_t s;

	switch(type) {
	default:
		s = sizeof(*o);
		break;

	case OBJECT_TYPE_BOULDER:
		s = sizeof(boulder);
		break;

	case OBJECT_TYPE_BOMB:
		s = sizeof(bomb);
		break;
	}

	o = malloc(s);

	if(o) {
		memset(o, 0, s);

		o->type = type;
		o->x = x;
		o->y = y;

		switch(type) {
		case OBJECT_TYPE_BOMB:
			o->passable = 1;
			break;

		case OBJECT_TYPE_BOULDER:
			((boulder*)o)->strength = 1000;
			break;
		}
	}

	return(o);
}

int game_init(int humans, int cpus)
{
	int ret_val;
	int x, y;
	int i;
	int n;

	ret_val = 0;
	n = humans + cpus;
	players = malloc(sizeof(*players) * n);

	if(!players) {
		ret_val = -ENOMEM;
		goto gtfo;
	}

	memset(players, 0, sizeof(*players) * n);
	nplayers = n;
	cpu = players + humans;

	if(n == 2) {
		SETPSPAWN(0, 1, 1);
		SETPSPAWN(1, WIDTH - 2, HEIGHT - 2);
	} else if(n == 3) {
		SETPSPAWN(0, 1, 1);
		SETPSPAWN(1, WIDTH - 2, 1);
		SETPSPAWN(2, WIDTH - 2, HEIGHT - 2);
	} else if(n == 4) {
		SETPSPAWN(0, 1, 1);
		SETPSPAWN(1, WIDTH - 2, 1);
		SETPSPAWN(2, 1, HEIGHT - 2);
		SETPSPAWN(3, WIDTH - 2, HEIGHT - 2);
	} else {
		/* player count not implemented */
		ret_val = -EINVAL;
	}

	for(i = 0; i < n; i++) {
		if(i < humans) {
			players[i].type = PLAYER_HUMAN;
		} else {
			players[i].type = PLAYER_CPU;
		}

		players[i].num = i;
		players[i].dx = 0;
		players[i].dy = 0;
		players[i].bomb_timeout = PLAYER_DEFAULT_TIMEOUT;
		players[i].bomb_strength = PLAYER_DEFAULT_STRENGTH;
		players[i].health = PLAYER_DEFAULT_HEALTH;
	}

	memset(&objects, 0, sizeof(objects));

	for(x = 0; x < WIDTH; x++) {
		for(y = 0; y < HEIGHT; y++) {
			if(IS_WALL(x, y)) {
				objects[x][y] = make_object(OBJECT_TYPE_WALL, x, y);
				assert(objects[x][y]);
			} else if(IS_BOULDER(x, y)) {
				objects[x][y] = make_object(OBJECT_TYPE_BOULDER, x, y);
				assert(objects[x][y]);
			} else if(IS_PILLAR(x, y)) {
				objects[x][y] = make_object(OBJECT_TYPE_PILLAR, x, y);
				assert(objects[x][y]);
			}
		}
	}

gtfo:
	if(ret_val < 0) {
		if(players) {
			free(players);
			players = NULL;
		}

		if(cpu) {
			cpu = NULL;
		}
	}

	return(ret_val);
}

object* game_object_at(const int x, const int y)
{
	if(x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) {
		return(NULL);
	}

	return(objects[x][y]);
}

player* game_player_num(const int n)
{
	if(n < 0 || n >= nplayers) {
		return(NULL);
	}

	return(players + n);
}

int game_num_players(void)
{
	return(nplayers);
}

void game_animate(void)
{
	int n;

	/* advance all animations one frame */

	for(n = 0; n < nplayers; n++) {
		player *p = players + n;

		if(p->dx > 0) {
			p->dx--;
		} else if(p->dx < 0) {
			p->dx++;
		}

		if(p->dy > 0) {
			p->dy--;
		}
		if(p->dy < 0) {
			p->dy++;
		}
	}

	return;
}

void game_player_move(const int p, const int dx, const int dy)
{
	int tx, ty;

	tx = PLX(p) + dx;
	ty = PLY(p) + dy;

	/* check for collision */
	if(!objects[tx][ty] || objects[tx][ty]->passable) {
		players[p].dx = -32 * dx;
		players[p].dy = -32 * dy;
		SETPPOS(p, tx, ty);
	}

	return;
}

void game_player_action(const int p)
{
	object *o;
	int px, py;

	px = PLX(p);
	py = PLY(p);

	o = make_object(OBJECT_TYPE_BOMB, px, py);

    if(o) {
		((bomb*)o)->strength = players[p].bomb_strength;
		((bomb*)o)->timeout = players[p].bomb_timeout * FPS;

		objects[px][py] = o;
	}

	return;
}

void game_logic(void)
{
	int x, y;

	for(x = 0; x < WIDTH; x++) {
		for(y = 0; y < HEIGHT; y++) {
			object *o;
			int tmp;

			o = objects[x][y];

			if(!o) {
				continue;
			}

			switch(objects[x][y]->type) {
			case OBJECT_TYPE_BOMB:
				tmp = --((bomb*)o)->timeout;

				if(tmp <= 0) {
					int dmg;
					int tx, ty;
					int i;

					/* detonate bomb N */

					for(dmg = ((bomb*)o)->strength, tx = x, ty = y;
						dmg > 0 && ty >= 0;
						ty--, dmg -= BOMB_GRADIENT) {
						object *target;

						target = objects[tx][ty];

						/*
						 * Check if there is a player a this position;
						 * the extra step is necessary since players
						 * are not in the objects map
						 */
						for(i = 0; i < nplayers; i++) {
							if(PLX(i) == tx && PLY(i) == ty) {
								players[i].health -= dmg;
							}
						}

						if(!target) {
							continue;
						}

						if(target->type == OBJECT_TYPE_BOULDER) {
							((boulder*)target)->strength -= dmg;
							break;
						}
					}

					/* detonate bomb S */

					for(dmg = ((bomb*)o)->strength, tx = x, ty = y;
						dmg > 0 && ty < HEIGHT;
						ty++, dmg -= BOMB_GRADIENT) {
						object *target;

						target = objects[tx][ty];

						for(i = 0; i < nplayers; i++) {
							if(PLX(i) == tx && PLY(i) == ty) {
								players[i].health -= dmg;
							}
						}

						if(!target) {
							continue;
						}

						if(target->type == OBJECT_TYPE_BOULDER) {
							((boulder*)target)->strength -= dmg;
							break;
						}
					}

					/* detonate bomb W */

					for(dmg = ((bomb*)o)->strength, tx = x, ty = y;
						dmg > 0 && tx >= 0;
						tx--, dmg -= BOMB_GRADIENT) {
						object *target;

						target = objects[tx][ty];

						for(i = 0; i < nplayers; i++) {
							if(PLX(i) == tx && PLY(i) == ty) {
								players[i].health -= dmg;
							}
						}

						if(!target) {
							continue;
						}

						if(target->type == OBJECT_TYPE_BOULDER) {
							((boulder*)target)->strength -= dmg;
							break;
						}
					}

					/* detonate bomb E */

					for(dmg = ((bomb*)o)->strength, tx = x, ty = y;
						dmg > 0 && tx < WIDTH;
						tx++, dmg -= BOMB_GRADIENT) {
						object *target;

						target = objects[tx][ty];

						for(i = 0; i < nplayers; i++) {
							if(PLX(i) == tx && PLY(i) == ty) {
								players[i].health -= dmg;
							}
						}

						if(!target) {
							continue;
						}

						if(target->type == OBJECT_TYPE_BOULDER) {
							((boulder*)target)->strength -= dmg;
							break;
						}
					}
				}
			}
		}
	}

	/* clean up destroyed elements */

	for(x = 0; x < WIDTH; x++) {
		for(y = 0; y < HEIGHT; y++) {
			object *o;

			o = objects[x][y];

			if(!o) {
				continue;
			}

			switch(o->type) {
			case OBJECT_TYPE_BOULDER:
				if(((boulder*)o)->strength <= 0) {
					free(o);
					objects[x][y] = NULL;
				}
				break;

			case OBJECT_TYPE_BOMB:
				if(((bomb*)o)->timeout <= 0) {
					free(o);
					objects[x][y] = NULL;
				}
				break;

			default:
				break;
			}
		}
	}

	for(x = 0; x < nplayers; x++) {
		if(players[x].health <= 0) {
			if(players[x].lifes > 0) {
				players[x].lifes--;
				players[x].health = PLAYER_DEFAULT_HEALTH;
				PLX(x) = players[x].spawn_x;
				PLY(x) = players[x].spawn_y;
			}
		}
	}


	return;
}
