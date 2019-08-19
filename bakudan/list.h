#ifndef LIST_H
#define LIST_H

typedef struct _list list;

struct _list {
	void *data;
	list *next;
};

#define LIST_FOREACH(lst,type,elem,what) do {							\
		list *__iter_##elem = *(lst);									\
		while(__iter_##elem) {											\
			type *elem = (type*)__iter_##elem->data;					\
			{															\
				what													\
					}													\
			__iter_##elem = __iter_##elem->next;						\
		}																\
	} while(0)

int list_append(list**, void*);
int list_remove(list**, void*);
void list_free(list**);

#endif /* LIST_H */
