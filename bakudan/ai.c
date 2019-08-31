#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "ai.h"
#include "game.h"
#include "list.h"

extern player *players;
extern player *cpu;
static ai _ai[4];
int num_ais;

#ifdef DEBUG_AI
#define DBG printf
#else /* DEBUG_AI */
#define DBG(...)
#endif /* DEBUG_AI */

static const char *_object_names[] = {
	"OBJECT_TYPE_WALL",
	"OBJECT_TYPE_PILLAR",
	"OBJECT_TYPE_BOULDER",
	"OBJECT_TYPE_ITEM",
	"OBJECT_TYPE_BOMB",
	"OBJECT_TYPE_PLAYER",
	"INVALID OBJECT"
};

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

static void _debug_path(ai_path*);
static struct pq* _safe_location(const int, const int, const int);

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

	if(sx < 0 || sy < 0 || dx < 0 || dy < 0 ||
	   sx >= WIDTH || sy >= HEIGHT ||
	   dx >= WIDTH || dy >= HEIGHT) {
		return(NULL);
	}

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

		if(cx == dx && cy == dy) {
			continue;
		}

#define CHECK_NEIGHBOR(_x, _y) do {									\
			if(!objects[_x][_y] || objects[_x][_y]->passable ||		\
			   (opts && ((_x) == dx && (_y) == dy))) {				\
				if(state[_x][_y].d == -1) {							\
					state[_x][_y].x = cx;							\
					state[_x][_y].y = cy;							\
					state[_x][_y].d = cd + 1;						\
					pq_insert(&q, _x, _y, cd + 1);					\
				}													\
			}														\
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
		} while(!(x == sx && y == sy));

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

static inline int _num_steps(const int ax, const int ay, const int bx, const int by)
{
	return((ax < bx ? bx - ax : ax - bx) +
		(ay < by ? by - ay : ay - by));
}

static void _debug_path(ai_path *path)
{
	int n;

	for(n = 0; path; n++, path = path->next) {
		printf("%d: (%02d, %02d)\n", n, path->x, path->y);
	}

	return;
}

struct pq* _safe_locations(const int px, const int py, const int risk)
{
	extern object *objects[WIDTH][HEIGHT];
	struct pq *ret_val;
	int x, y;

	ret_val = NULL;

	for(x = 1; x < WIDTH - 1; x++) {
		for(y = 1; y < HEIGHT - 1; y++) {
			object *o;

			o = objects[x][y];

			if(o && !o->passable) {
				continue;
			}

			if(!game_location_dangerous(x, y, risk)) {
				pq_insert(&ret_val, x, y, _num_steps(px, py, x, y));
			}
		}
	}

	return(ret_val);
}

list* _targets_within(const int self, const int x, const int y, const int steps)
{
	extern object *objects[WIDTH][HEIGHT];
	int lx, ly, ux, uy, tx, ty;
	list *ret_val;

	ret_val = NULL;

	/*
	 * The list that is returned contains pointers to the object table
	 * or player array. This has the advantage that we don't have to deal
	 * with objects that have been removed from the object table between
	 * the invocation of this function and the time the list is checked.
	 * Also, if an object is removed from the object table but another
	 * object is inserted in the same location, the list will still
	 * correctly refer to a valid object (since the distance is still the
	 * same). [This may happen in some rare cases, like a player dying
	 * and dropping a life while collecting an item, causing the item
	 * to be replaced with a life.]
	 */

	lx = MAX(x - steps, 1);
	ly = MAX(y - steps, 1);
	ux = MIN(x + steps, WIDTH - 1);
	uy = MIN(y + steps, WIDTH - 1);

	for(tx = lx; tx < ux; tx++) {
		for(ty = ly; ty < uy; ty++) {
			object *o;

			if(_num_steps(x, y, tx, ty) > steps) {
				continue;
			}

			o = objects[tx][ty];

			if(o && (o->type == OBJECT_TYPE_BOULDER ||
					 o->type == OBJECT_TYPE_ITEM)) {
				/*
				 * add a pointer to the pointer to the object
				 * instead of a pointer to the object, so we
				 * can forget about mutexes and synchronization
				 */
				DBG("list_append(%p, %p [&(objects[%d][%d])])\n", &ret_val, &(objects[tx][ty]), tx, ty);
				list_append(&ret_val, &(objects[tx][ty]));
			}
		}
	}

	for(tx = 0; tx < game_num_players(); tx++) {
		if(tx == self) {
			/* don't include oneself in the list of targets */
			continue;
		}

		lx = obj_x(&(players[tx]));
		ly = obj_x(&(players[tx]));

		if(_num_steps(x, y, lx, ly) < steps) {
			list_append(&ret_val, &(players[tx]));
		}
	}

	return(ret_val);
}

void _ai_think(ai *me)
{
	int d;
	list *targets;
	int x, y;
	int risk;

	if(game_player_moving(me->self)) {
		/* don't waste CPU cycles while we can't do anything anyways */
		return;
	}

	game_player_location(me->self, &x, &y);
	risk = (int)((float)players[me->self].health * me->tolerance);

	/* first of all, make sure we're not in danger */

	if(game_location_dangerous(x, y, risk)) {
		struct pq *locs;
		int dx, dy;

		printf("Need to flee from (%02d, %02d)\n", x, y);

		locs = _safe_locations(x, y, risk);

		while(locs) {
			ai_path *path;

			printf("Found safe location (%02d, %02d)\n", locs->x, locs->y);

			path = ai_find_path(x, y, locs->x, locs->y, 0);

			if(path) {
				printf("Found a path to (%02d, %02d) via (%02d, %02d)\n",
					   locs->x, locs->y, path->x, path->y);
				_debug_path(path);

				game_player_move_abs(me->self, path->x, path->y);
				ai_path_free(&path);
				pq_free(&locs);

				return;
			} else {
				struct pq *free_me;

				free_me = locs;
				locs = locs->next;
				free(free_me);
			}
		}

		/*
		 * Couldn't find a place to flee to - continue as usual
		 * since there's nothing we can do anyways
		 */
	}

	for(d = 1; d < MAX(WIDTH, HEIGHT); d++) {
		object **o;
		int done;

		done = 0;
		targets = _targets_within(me->self, x, y, d);

		if(!targets) {
			/* no targets within `d' steps */
			DBG("No targets within %d steps\n", d);
			continue;
		}

		DBG("Have targets within %d steps\n", d);

		while((o = (object**)list_pop(&targets)) && !done) {
			ai_path *path;

			if(*o) {
				object_type ot;
				int ox, oy;

			    DBG("%p [(%02d, %02d), %d]\n",
					*o, (*o)->x,
					(*o)->y, (*o)->type);

				ox = (*o)->x;
				oy = (*o)->y;
				ot = (*o)->type;

				path = ai_find_path(x, y, ox, oy,
									ot == OBJECT_TYPE_BOULDER ? 1 : 0);

				if(path) {
					/* if we have a path, walk it */

					if(!game_location_dangerous(path->x, path->y, risk)) {
						DBG("Next step in path: (%02d,%02d)\n", path->x, path->y);

						if(path->x == x && path->y == y) {
							/* this happens with boulders */
							game_player_action(me->self);
						} else {
							game_player_move_abs(me->self, path->x, path->y);
						}

						done = 1;
					}

					ai_path_free(&path);
				} else {
					/*
					 * If we don't have a path it may be because there is none, or
					 * because we have already arrived (this is the case with boulders,
					 * where the last step in the path is not the location of the target)
					 */

					DBG("Target at (%02d,%02d) is a %s\n",
						ox, oy,	_object_names[ot]);

					if(_num_steps(x, y, ox, oy) <= 1) {
						/* place bomb */
						DBG("Close enough, place bomb\n");
						game_player_action(me->self);
						done = 1;
					} else {
						/* there really is no path */
						DBG("Found a target, but no path to it\n");
					}
				}
			}
		}

		while(list_pop(&targets));
	}

	return;
}

void ai_path_free(ai_path **path)
{
	while(*path) {
		ai_path *free_me;

		free_me = *path;
		*path = free_me->next;
		free(free_me);
	}

	return;
}

void ai_tick(void)
{
	int i;

	for(i = 0; i < num_ais; i++) {
		if(!cpu[i].alive) {
			continue;
		}

		_ai_think(&(_ai[i]));
	}

	return;
}
