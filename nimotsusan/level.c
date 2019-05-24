#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "level.h"

int readlvl(struct level **dst, const char *path)
{
	char line[256];
	struct level *lvl;
    FILE *fd;
	int width;
	int height;
	int x;
	int y;
	int ret_val;

	fd = fopen(path, "r");

	if(!fd) {
		ret_val = -errno;
		perror("fopen");
		goto gtfo;
	}

	width = 0;
	height = 0;
	x = 0;
	y = 0;

	lvl = malloc(sizeof(*lvl));

	if(!lvl) {
		ret_val = -ENOMEM;
		goto gtfo;
	}

	memset(lvl, 0, sizeof(*lvl));

	while(fgets(line, sizeof(line), fd)) {
		size_t len;

		len = strlen(line);

		if(!width) {
			width = len;
		} else if(len > 0 && width != len) {
			/* size mismatch */
			break;
		}

		for(x = 0; x < width; x++) {
			struct object *o;
			obj_type type;

			switch(line[x]) {
			case '#':
				type = OBJ_KABE;
				break;

			case 'p':
				type = OBJ_HITO;
				break;

			case '.':
				type = OBJ_KUCHI;
				break;

			case 'o':
				type = OBJ_HAKO;
				break;

			case 'O':
				type = OBJ_MARU;
				break;

			default:
				type = OBJ_UNKNOWN;
				break;
			}

			if(type != OBJ_UNKNOWN) {
				struct object *o;

				o = malloc(sizeof(*o));

				if(o) {
					memset(o, 0, sizeof(*o));

					o->type = type;
					o->x = x;
					o->y = y;
					o->next = lvl->objects;
					lvl->objects = o;
					lvl->nobjects++;

					if(o->type == OBJ_HITO) {
						lvl->player = o;
					}
				}
			}
		}

		y++;
		height = y;
	}

	lvl->width = width;
	lvl->height = height;

	gtfo:
	if(fd) {
		fclose(fd);
		fd = NULL;
	}

	if(ret_val < 0) {
		if(lvl) {
			while(lvl->objects) {
				struct object *next;

				next = lvl->objects->next;
				free(lvl->objects);
				lvl->objects = next;
			}

			free(lvl);
			lvl = NULL;
		}
	}

	*dst = lvl;

	return(ret_val);
}
