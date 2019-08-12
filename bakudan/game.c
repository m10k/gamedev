#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "game.h"
#include "engine.h"

static player *players = NULL;
static player *cpu = NULL;
static int nplayers = 0;
static object *objects[WIDTH][HEIGHT];
static int alive_players;

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

	case OBJECT_TYPE_ITEM:
		s = sizeof(item);
		break;
	}

	o = malloc(s);

	if(o) {
		memset(o, 0, s);

		o->type = type;
		o->x = x;
		o->y = y;

		switch(type) {
		case OBJECT_TYPE_ITEM:
		case OBJECT_TYPE_BOMB:
			o->passable = 1;
			break;

		case OBJECT_TYPE_BOULDER:
			((boulder*)o)->strength = BOULDER_DEFAULT_STRENGTH;
			break;
		}
	}

	return(o);
}

static void drop_item(const int x, const int y)
{
	item_type type;
	item *i;

	type = game_ask_universe2(0, ITEM_TYPE_NUM - 1);

	i = (item*)make_object(OBJECT_TYPE_ITEM, x, y);

	if(i) {
		i->type = type;

		switch(type) {
		case ITEM_TYPE_BAG:
			i->bombs = 1;
			break;

		case ITEM_TYPE_LIFE:
			i->lifes = 1;
			break;

		case ITEM_TYPE_LUCK:
			i->probability = game_ask_universe2(-10, 10);
			break;

		case ITEM_TYPE_POTION:
			i->health = game_ask_universe2(10, 100);
			break;

		case ITEM_TYPE_TIME:
			i->health = game_ask_universe2(-3, 3);
			break;

		case ITEM_TYPE_POWER:
			i->health = game_ask_universe2(-500, 500);
			break;

		default:
			assert(0);
			break;
		}

		objects[x][y] = (object*)i;
	}

	return;
}

void drop_life(const int x, const int y)
{
	item *i;

	i = (item*)make_object(OBJECT_TYPE_ITEM, PLX(x), PLY(x));

	if(i) {
		i->lifes = 1;
		objects[x][y] = (object*)i;
	}

	return;
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
	alive_players = n;
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
		players[i].alive = 1;

		players[i].bomb_timeout = PLAYER_DEFAULT_TIMEOUT;
		players[i].bomb_strength = PLAYER_DEFAULT_STRENGTH;
		players[i].health = PLAYER_DEFAULT_HEALTH;
		players[i].lifes = PLAYER_DEFAULT_LIFES;
		players[i].probability = PLAYER_DEFAULT_PROBABILITY;
		players[i].bombs = PLAYER_DEFAULT_BOMBS;
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

	if(players[p].bombs > 0) {
		px = PLX(p);
		py = PLY(p);

		o = make_object(OBJECT_TYPE_BOMB, px, py);

		if(o) {
			((bomb*)o)->strength = players[p].bomb_strength;
			((bomb*)o)->timeout = players[p].bomb_timeout * FPS;
			((bomb*)o)->owner = p;

			objects[px][py] = o;
		}

		players[p].bombs--;
	}

	return;
}

int bomb_strength_at(bomb *b, const int x, const int y)
{
	int dx, dy;
	int dist;
	int dmg;

	dx = obj_x(b) - x;
	dy = obj_y(b) - y;
    dist = 1000;

	if(dx == 0) {
		dist = dy < 0 ? -dy : dy;
	} else if(dy == 0) {
		dist = dx < 0 ? -dx : dx;
	}

	dmg = b->strength - (dist * BOMB_GRADIENT);

	if(dmg < 0) {
		dmg = 0;
	}

	return(dmg);
}

void player_damage(const int p, const int dmg, bomb *b)
{
	if(players[p].health > 0) {
		printf("Dealing %d dmg to player %d (newhp: %d)\n", dmg, p, players[p].health - dmg);
		players[p].health -= dmg;
		players[p].attacker = b->owner;
	}

	return;
}

void boulder_damage(object *o, const int dmg, bomb *b)
{
	boulder *bld = (boulder*)o;

	if(bld->strength > 0) {
		printf("Dealing %d dmg to boulder (%d, %d) (newstr: %d)\n", dmg, o->x, o->y,
			   bld->strength - dmg);
		bld->strength -= dmg;
		bld->attacker = b->owner;
	}

	return;
}

void bomb_detonate(bomb *b)
{
	int tx, ty;
	int i;

	for(tx = obj_x(b) - 1, ty = obj_y(b);
		tx > 0; tx--) {
		object *o;
		int dmg;

		dmg = bomb_strength_at(b, tx, ty);

		if(dmg <= 0) {
			break;
		}

		for(i = 0; i < nplayers; i++) {
			if(PLX(i) == tx && PLY(i) == ty) {
				player_damage(i, dmg, b);
			}
		}

		o = objects[tx][ty];

		if(!o) {
			continue;
		}

		if(o->type == OBJECT_TYPE_BOULDER) {
			boulder_damage(o, dmg, b);
		} else if(o->type == OBJECT_TYPE_WALL ||
				  o->type == OBJECT_TYPE_PILLAR) {
			/* stop detonation in this direction */
			break;
		}
	}

	for(tx = obj_x(b) + 1, ty = obj_y(b);
		tx < WIDTH; tx++) {
		object *o;
		int dmg;

		dmg = bomb_strength_at(b, tx, ty);

		if(dmg <= 0) {
			break;
		}

		for(i = 0; i < nplayers; i++) {
			if(PLX(i) == tx && PLY(i) == ty) {
				player_damage(i, dmg, b);
			}
		}

		o = objects[tx][ty];

		if(!o) {
			continue;
		}

		if(o->type == OBJECT_TYPE_BOULDER) {
			boulder_damage(o, dmg, b);
		} else if(o->type == OBJECT_TYPE_WALL ||
				  o->type == OBJECT_TYPE_PILLAR) {
			/* stop detonation in this direction */
			break;
		}
	}

	for(tx = obj_x(b), ty = obj_y(b) - 1;
		ty > 0; ty--) {
		object *o;
		int dmg;

		dmg = bomb_strength_at(b, tx, ty);

		if(dmg <= 0) {
			break;
		}

		for(i = 0; i < nplayers; i++) {
			if(PLX(i) == tx && PLY(i) == ty) {
				player_damage(i, dmg, b);
			}
		}

		o = objects[tx][ty];

		if(!o) {
			continue;
		}

		if(o->type == OBJECT_TYPE_BOULDER) {
			boulder_damage(o, dmg, b);
		} else if(o->type == OBJECT_TYPE_WALL ||
				  o->type == OBJECT_TYPE_PILLAR) {
			/* stop detonation in this direction */
			break;
		}
	}

	for(tx = obj_x(b), ty = obj_y(b) + 1;
		ty < HEIGHT; ty++) {
		object *o;
		int dmg;

		dmg = bomb_strength_at(b, tx, ty);

		if(dmg <= 0) {
			break;
		}

		for(i = 0; i < nplayers; i++) {
			if(PLX(i) == tx && PLY(i) == ty) {
				player_damage(i, dmg, b);
			}
		}

		o = objects[tx][ty];

		if(!o) {
			continue;
		}

		if(o->type == OBJECT_TYPE_BOULDER) {
			boulder_damage(o, dmg, b);
		} else if(o->type == OBJECT_TYPE_WALL ||
				  o->type == OBJECT_TYPE_PILLAR) {
			/* stop detonation in this direction */
			break;
		}
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
					bomb_detonate((bomb*)o);
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

					/* decide whether to spawn an item */
					if(game_ask_universe(players[((boulder*)o)->attacker].probability)) {
						printf("Dropping item at (%d, %d)\n", x, y);
						drop_item(x, y);
					}

					players[((boulder*)o)->attacker].boulders++;
				}
				break;

			case OBJECT_TYPE_BOMB:
				if(((bomb*)o)->timeout <= 0) {
					/* allow owner to spawn another bomb */
					players[((bomb*)o)->owner].bombs++;

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
		if(players[x].alive) {
			if(players[x].health <= 0) {
				printf("Player %d killed %d\n", players[x].attacker, x);

				players[x].deaths++;
				players[players[x].attacker].frags++;

				if(players[x].attacker == x) {
					players[x].suicides++;
				}

				/* drop a life? */
				if(game_ask_universe(50)) {
					drop_life(PLX(x), PLY(x));
				}

				if(players[x].lifes > 0) {
					players[x].lifes--;
					players[x].health = PLAYER_DEFAULT_HEALTH;
					PLX(x) = players[x].spawn_x;
					PLY(x) = players[x].spawn_y;
				} else {
					players[x].alive = 0;
					alive_players--;
				}
			}
		}
	}

	if(alive_players < 2) {
		/* game over */
		engine_set_state(GAME_STATE_SBOARD);
	}

	return;
}

int game_ask_universe(int prob)
{
	int fd;
	unsigned char rnd;

	fd = open("/dev/urandom", O_RDONLY);

	if(fd >= 0) {
		int err;

		err = read(fd, &rnd, 1);

		if(err > 0) {
			rnd = rnd % 100;
		}

		close(fd);

		if(err < 0) {
			/* failed to read from /dev/urandom, so fall back to srand */
			fd = -1;
		}
	}

	if(fd < 0) {
		static int _sr_initialized = 0;

		printf("Falling back to srand()\n");

		if(!_sr_initialized) {
			srand(time(NULL));
			_sr_initialized = 1;
		}

		rnd = rand() % 100;
	}

	printf("%d < %d\n", rnd, prob);

	return(rnd < prob ? 1 : 0);
}

int game_ask_universe2(const int l, const int u)
{
	int fd;
	int rnd;

	fd = open("/dev/urandom", O_RDONLY);

	if(fd >= 0) {
		int err;

		err = read(fd, &rnd, sizeof(rnd));

		close(fd);

		if(err < 0) {
			/* failed to read from /dev/urandom, so fall back to srand */
			fd = -1;
		}
	}

	if(fd < 0) {
		static int _sr_initialized = 0;

		printf("Falling back to srand()\n");

		if(!_sr_initialized) {
			srand(time(NULL));
			_sr_initialized = 1;
		}

		rnd = rand();
	}

	rnd = (rnd % (u - l)) + l;

	return(rnd);
}