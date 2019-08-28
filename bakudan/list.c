#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "list.h"

int list_append(list **l, void *data)
{
	list *item;
	int ret_val;

	ret_val = -ENOMEM;
	item = malloc(sizeof(*item));

	if(item) {
	    item->data = data;
		item->next = NULL;

		while(*l) {
			l = &((*l)->next);
		}

		*l = item;
		ret_val = 0;
	}

	return(ret_val);
}

int list_remove(list **l, void *data)
{
	int ret_val;

	ret_val = -ENOENT;

	while(*l) {
		if((*l)->data == data) {
			list *free_me;

			free_me = *l;
			*l = (*l)->next;

			free(free_me);
			ret_val = 0;
			break;
		}
	}

	return(ret_val);
}

void* list_pop(list **l)
{
	list *item;
	void *ret_val;

	item = *l;
	ret_val = NULL;

	if(item) {
		ret_val = item->data;
		*l = item->next;
		free(item);
	}

	return(ret_val);
}

void list_free(list **l)
{
	while(*l) {
		list *free_me;

		free_me = *l;
		l = &((*l)->next);

		free(free_me);
	}

	return;
}
