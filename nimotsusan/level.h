#ifndef LEVEL_H
#define LEVEL_H

typedef enum obj_type {
	OBJ_NONE = 0,
	OBJ_HAKO,
	OBJ_KUCHI,
	OBJ_HITO,
	OBJ_MARU,
	OBJ_KABE,
	OBJ_UNKNOWN
} obj_type;

struct object {
	struct object *next;
	int x;
	int y;
	obj_type type;
};

struct level {
	int width;
	int height;
	int nobjects;

	struct object *objects;
	struct object *player;
};

int readlvl(struct level**, const char*);

#endif /* LEVEL_H */
