#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "ai.h"
#include "game.h"
#include "list.h"

extern list *bombs;
extern player *players;
extern player *cpu;
static ai _ai[4];
int num_ais;

struct pq {
	int x;
	int y;
	int d;
	struct pq *next;
};

/*
 *  0123456789ABCDE
 * 0###############0
 * 1#! @@@@@@@@@ !#1
 * 2# *@*@*@*@*@* #2
 * 3#@@@@@@@@@@@@@#3
 * 4#@*@*@*@*@*@*@#4
 * 5#@@@@@@@@@@@@@#5
 * 6#@*@*@*@*@*@*@#6
 * 7#@@@@@@@@@@@@@#7
 * 8#@*@*@*@*@*@*@#8
 * 9#@@@@@@@@@@@@@#9
 * A#@*@*@*@*@*@*@#A
 * B#@@@@@@@@@@@@@#B
 * C# *@*@*@*@*@* #C
 * D#! @@@@@@@@@ !#D
 * E###############E
 *  0123456789ABCDE
 */

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

object* ai_find_closest(const object_type type, const int x, const int y)
{
	extern object *objects[WIDTH][HEIGHT];
	int dia;
	int dir;

	for(dia = 1; dia < MAX(WIDTH, HEIGHT) - 2; dia++) {
		int lx, ly, ux, uy;
		int tx, ty;

		/* prevent oob accesses to the objects array */
		lx = MAX(x - dia, 0);
		ly = MAX(y - dia, 0);
		ux = MIN(x + dia, WIDTH);
		uy = MIN(y + dia, HEIGHT);

		/*
		 * #### U
		 * #  #
		 * #  #
		 * #### D
		 */

		/* #### U */
		for(tx = lx; tx <= ux; tx++) {
			object *o;

			o = objects[tx][ly];

			if(o && o->type == type) {
				return(o);
			}
		}

		/*
		 * #  #
		 * #  #
		 */
		for(ty = ly + 1; ty < uy; ty++) {
			object *o;

			o = objects[lx][ty];

			if(o && o->type == type) {
				return(o);
			}

			o = objects[ux][ty];

			if(o && o->type == type) {
				return(o);
			}
		}

		/* #### D */
		for(tx = lx; tx < ux; tx++) {
			object *o;

			o = objects[tx][uy];

			if(o && o->type == type) {
				return(o);
			}
		}
	}

	return(NULL);
}

#define IN_BOUNDS(_a,_b) (((_a) > 0 && (_a) < WIDTH) && \
						  ((_b) > 0 && (_b) < HEIGHT))

object* ai_find_closest2(const object_type type, const int x, const int y)
{
	extern object* objects[WIDTH][HEIGHT];
	int dist;

	for(dist = 1; dist < WIDTH + HEIGHT; dist++) {
		int a, b;

		for(a = dist, b = 0; a >= 0; a--, b++) {
#define CHECK(_a,_b) do {							\
				if(IN_BOUNDS((_a), (_b))) {			\
					object *o = objects[_a][_b];	\
					if(o && o->type == type) {		\
						return(o);					\
					}								\
				}								\
			} while(0)

			CHECK(x + a, y + b);
			CHECK(x + a, y - b);
			CHECK(x - a, y + b);
			CHECK(x - a, y - b);

#undef CHECK
		}
	}

	return(NULL);
}

int ai_find_refugee(const int x, const int y, const int tolerance, int *dx, int *dy)
{
	extern object* objects[WIDTH][HEIGHT];
	int dist;

	for(dist = 1; dist < WIDTH + HEIGHT; dist++) {
		int a, b;

		for(a = dist, b = 0; a >= 0; a--, b++) {
#define CHECK(_a,_b) do {												\
				if(IN_BOUNDS((_a), (_b))) {								\
					object *o = objects[_a][_b];						\
					if(!o || (o->passable &&							\
							  o->type != OBJECT_TYPE_BOMB)) {			\
						if(!game_location_dangerous((_a), (_b), tolerance)) { \
							*dx = (_a);									\
							*dy = (_b);									\
							return(0);									\
						}												\
					} \
				} \
			} while(0)

			CHECK(x + a, y + b);
			CHECK(x + a, y - b);
			CHECK(x - a, y + b);
			CHECK(x - a, y - b);

#undef CHECK
		}
	}

	return(-ENOENT);
}

int ai_path_length(ai_path *p)
{
	int ret_val;

	ret_val = -EINVAL;

	if(p) {
		ret_val = 0;

		while(p) {
			p = p->next;
			ret_val++;
		}
	}

	return(ret_val);
}

void pq_insert(struct pq **head, const int x, const int y, const int d)
{
	struct pq *item;

	item = malloc(sizeof(*item));

	if(item) {
		struct pq *cur;

		item->x = x;
		item->y = y;
		item->d = d;

		while(*head && (*head)->d < d) {
			head = &((*head)->next);
		}

		item->next = *head;
		*head = item;
	}

	return;
}

struct pq* pq_head(struct pq **head)
{
	struct pq *item;

	item = *head;

	if(item) {
		*head = item->next;
	}

	return(item);
}

void pq_free(struct pq **head)
{
	while(*head) {
		struct pq *next;

		next = (*head)->next;
		free(*head);
		*head = next;
	}

	return;
}

struct dijkstra_state {
	int x;
	int y;
	int d;
};

ai_path* ai_find_path(const int sx, const int sy,
					  const int dx, const int dy,
					  const int opts)
{
	extern object *objects[WIDTH][HEIGHT];

	struct dijkstra_state state[WIDTH][HEIGHT];
	ai_path *ret_val;
	ai_path **next;
	struct pq *q;
	struct pq *cq;
	int x, y;

	for(x = 0; x < WIDTH; x++) {
		for(y = 0; y < HEIGHT; y++) {
			state[x][y].x = -1;
			state[x][y].y = -1;
			state[x][y].d = -1;
		}
	}

	q = NULL;
	ret_val = NULL;
	next = &(ret_val);

	pq_insert(&q, sx, sy, 0);
	state[sx][sy].x = sx;
	state[sx][sy].y = sy;
	state[sx][sy].d = 0;

	/* while the queue isn't empty */
	while((cq = pq_head(&q))) {
		int tx, ty;
		int cx, cy, cd;

		cx = cq->x;
		cy = cq->y;
		cd = cq->d;

#define CHECK_NEIGHBOR(_x, _y) do {								\
			if(!objects[_x][_y] || objects[_x][_y]->passable || \
			   (opts && ((_x) == dx && (_y) == dy))) {			\
				if(state[_x][_y].d == -1) {						\
					state[_x][_y].x = cx;						\
					state[_x][_y].y = cy;						\
					state[_x][_y].d = cd + 1;					\
					pq_insert(&q, _x, _y, cd + 1);				\
				}												\
			}													\
		} while(0)

		CHECK_NEIGHBOR(cq->x, cq->y - 1);
		CHECK_NEIGHBOR(cq->x - 1, cq->y);
		CHECK_NEIGHBOR(cq->x + 1, cq->y);
		CHECK_NEIGHBOR(cq->x, cq->y + 1);

#undef CHECK_NEIGHBOR
	}

	if(state[dx][dy].d >= 0) {
		/* destination is reachable */
		ai_path *path;

		path = NULL;

		/* omit last step if opts is set */
		if(!opts) {
			x = dx;
			y = dy;
		} else {
			x = state[dx][dy].x;
			y = state[dx][dy].y;
		}

		do {
			ai_path *segm;
			int tx, ty;

			segm = malloc(sizeof(*segm));
			assert(segm);

			segm->x = x;
			segm->y = y;

			segm->next = path;
			path = segm;

			tx = state[x][y].x;
			ty = state[x][y].y;

			x = tx;
			y = ty;
		} while(x != sx && y != sy);

#if 0
		while(path) {
			ai_path *next;

			next = path->next;

			printf("(%02d,%02d)%s", path->x, path->y, next ? "->" : "\n");

			free(path);
			path = next;
		}

		for(y = 0; y < HEIGHT; y++) {
			for(x = 0; x < WIDTH; x++) {
				printf("(%02d,%02d) ", state[x][y].x, state[x][y].y);
			}
			printf("\n");
		}
#endif

		ret_val = path;
	}

	pq_free(&q);

	return(ret_val);
}

static void _ai_find_objective(ai *me)
{
	object *o;
	int sx, sy;
	int p;

	sx = obj_x(&(players[me->self]));
	sy = obj_y(&(players[me->self]));

	/* path to enemy? */
	for(p = 0; p < game_num_players(); p++) {
		ai_path *path;
		int px, py;

		if(p == me->self) {
			continue;
		}

		px = obj_x(&(players[p]));
		py = obj_y(&(players[p]));

		path = ai_find_path(sx, sy, px, py, 1);

		if(!path) {
			continue;
		}

		/* there is a path */

		me->obj.type = OBJECTIVE_KILL;
		me->obj.path = path;
		me->obj.target = p;
		me->obj.x = px;
		me->obj.y = py;

		me->have_obj = 1;
		return;
	}

	/* no path to any enemy, find a boulder to destroy */

	o = ai_find_closest2(OBJECT_TYPE_BOULDER, sx, sy);

	if(o) {
		printf("Found a boulder to destroy at (%02d, %02d)\n",
			   o->x, o->y);

		me->obj.type = OBJECTIVE_BOMB;
		me->obj.path = ai_find_path(sx, sy, o->x, o->y, 1);
		me->obj.x = o->x;
		me->obj.y = o->y;

		if(me->obj.path) {
			me->have_obj = 1;
			return;
		} else {
			printf("No path to boulder\n");
		}
	}

	printf("Nothing to do\n");

	return;
}

int ai_init(int n, int first)
{
	int ret_val;
	int i;

	ret_val = -EINVAL;

	if(n < 4) {
		num_ais = n;

		for(i = 0; i < n; i++) {
			_ai[i].self = first + i;
			_ai[i].have_obj = 0;
			_ai[i].tolerance = AI_DEFAULT_TOLERANCE;
		}

		ret_val = 0;
	}

	return(ret_val);
}

#define PLAYER_MOVING(pid) (players[pid].dx || players[pid].dy)

void _ai_think(ai *me)
{
	if(!me->have_obj) {
		_ai_find_objective(me);
	}

	if(me->have_obj) {
		ai_path *path;
		int x, y;

		/* make sure objective is still valid */

		switch(me->obj.type) {
		case OBJECTIVE_KILL:
			x = obj_x(&(players[me->obj.target]));
			y = obj_y(&(players[me->obj.target]));

			if(x != me->obj.x || y != me->obj.y) {
				while(me->obj.path) {
					path = me->obj.path;
					me->obj.path = path->next;
					free(path);
				}

				me->obj.path = ai_find_path(obj_x(&(players[me->self])),
											obj_y(&(players[me->self])),
											x, y, 1);

				if(!me->obj.path) {
					me->have_obj = 0;
					return;
				}
			}

			break;

		case OBJECTIVE_BOMB:

		case OBJECTIVE_ITEM:

		case OBJECTIVE_HIDE:
			break;
		}

		path = me->obj.path;

		if(path) {
			if(!game_player_moving(me->self)) {
				game_player_move_abs(me->self, path->x, path->y);

				me->obj.path = path->next;
				free(path);
			}
		} else {
			switch(me->obj.type) {
			case OBJECTIVE_ITEM:
				/* nothing to do */
				me->have_obj = 0;
				break;

			case OBJECTIVE_KILL:
			case OBJECTIVE_BOMB:
				/* plant bomb */

				if(game_player_can_plant(me->self)) {
					game_player_action(me->self);
					me->obj.type = OBJECTIVE_HIDE;
				} else {
					break;
				}

			case OBJECTIVE_HIDE:
				/* find safe place */

				if(!me->obj.path) {
					int tolerance;

					x = obj_x(&(players[me->self]));
					y = obj_y(&(players[me->self]));
					tolerance = (int)((float)players[me->self].health * me->tolerance);

					if(game_location_dangerous(x, y, tolerance)) {
						int dx, dy;
						int err;

						err = ai_find_refugee(x, y, tolerance, &dx, &dy);

						if(err < 0) {
							/* no place to hide */
							me->have_obj = 0;
						} else {
							me->obj.path = ai_find_path(x, y, dx, dy, 0);
						}
					} else {
						/* bomb's effect does not exceed our tolerance -> stay */
						me->have_obj = 0;
					}
				}

				break;

			}
		}
	}

	return;
}

void ai_tick(void)
{
	static char tick = 0;
	int i;

	if(++tick & 0x80) {
		tick = 0;
		printf("%s()\n", __func__);
	}

	for(i = 0; i < num_ais; i++) {
		if(!cpu[i].alive) {
			continue;
		}

		_ai_think(&(_ai[i]));
	}

	return;
}
