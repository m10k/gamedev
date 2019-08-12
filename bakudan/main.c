#include <stdio.h>
#include <string.h>
#include "engine.h"

int main(int argc, char *argv[])
{
	int ret_val;

	ret_val = engine_init();

	if(ret_val < 0) {
		fprintf(stderr, "game_init: %s\n", strerror(-ret_val));
	} else {
		ret_val = engine_run();

		if(ret_val < 0) {
			fprintf(stderr, "engine_run: %s\n", strerror(-ret_val));
		}

		engine_quit();
	}

	return(ret_val);
}
