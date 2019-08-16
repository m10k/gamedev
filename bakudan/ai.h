#ifndef AI_H
#define AI_H

typedef struct _ai_path ai_path;

struct _ai_path {
	ai_path *next;

	int x;
	int y;
};

typedef enum {
	OBJECTIVE_KILL,
	OBJECTIVE_BOMB,
	OBJECTIVE_ITEM,
	OBJECTIVE_HIDE
} objective_type;

typedef struct {
	objective_type type;
	ai_path *path;
	int target;
	int x;
	int y;
} objective;

typedef struct {
	int self;
	objective obj;
	int have_obj;
} ai;

int ai_init(const int, const int);
void ai_tick(void);

ai_path* ai_find_path(const int, const int, const int, const int);

#endif /* AI_H */
