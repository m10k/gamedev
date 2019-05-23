#include <stdio.h>

#define WIDTH  8
#define HEIGHT 8
#define NOBJS  2

static int stop = 0;
static char board[WIDTH][HEIGHT];

struct object {
	int x;
	int y;
	char type;
};

static struct object obj[(NOBJS * 2) + 1];
static int dx;
static int dy;

static void init(void)
{
	obj[0].x = 1;
	obj[0].y = 1;
	obj[0].type = '.';
	obj[1].x = 6;
	obj[1].y = 1;
	obj[1].type = '.';
	obj[2].x = 3;
	obj[2].y = 3;
	obj[2].type = 'o';
	obj[3].x = 3;
	obj[3].y = 4;
	obj[3].type = 'o';
	obj[4].x = 4;
	obj[4].y = 4;
	obj[4].type = 'p';

	return;
}

static void input(void)
{
    int c;

	dx = 0;
	dy = 0;

	c = getchar();

	switch(c) {
	case 'w':
		dy = -1;
		break;

	case 'a':
		dx = -1;
		break;

	case 's':
		dy = 1;
		break;

	case 'd':
		dx = 1;
		break;

	default:
		break;
	}

	return;
}

static int unpassable(int x, int y)
{
	int z;

	if(!x || !y || x == (WIDTH - 1) || y == (HEIGHT - 1)) {
		return(1);
	}

	for(z = 0; z < NOBJS * 2 + 1; z++) {
		if(obj[z].x == x &&
		   obj[z].y == y &&
		   obj[z].type != '.') {
			return(1);
		}
	}

	return(0);
}

static int occupied_by(int x, int y)
{
	int z;

	for(z = 0; z < NOBJS * 2 + 1; z++) {
		if(obj[z].x == x &&
		   obj[z].y == y) {
			return(z);
		}
	}

	return(-1);
}

static void move_object(int o, int x, int y)
{
	obj[o].x = x;
	obj[o].y = y;

	return;
}

static void process(void)
{
	int px, py;
	int rem;

	px = obj[4].x + dx;
	py = obj[4].y + dy;

	if(unpassable(px, py)) {
		int nx, ny;
		int ni;

		nx = px + dx;
		ny = py + dy;
		ni = occupied_by(px, py);

		/* check if occupator is not a wall and if it can be moved */
		if(ni >= 0 && !unpassable(nx, ny)) {
			int d;

			d = occupied_by(nx, ny);

			move_object(ni, nx, ny);
			move_object(4, px, py);

			/* passable but occupied means the nimotsu is on the destination */
			if(d >= 0) {
				obj[ni].type = 'O';
			}
		}
	} else {
		move_object(4, px, py);
	}

	for(px = 0, rem = 0; px < NOBJS * 2 + 1; px++) {
		if(obj[px].type == 'o') {
			rem++;
		}
	}

	if(!rem) {
		stop = 1;
	}

	return;
}

static void output(void)
{
	int x, y;


	for(y = 0; y < HEIGHT; y++) {
		for(x = 0; x < WIDTH; x++) {
			if(!x || !y || x == (WIDTH - 1) || y == (HEIGHT - 1)) {
				board[x][y] = '#';
			} else {
				board[x][y] = ' ';
			}
		}
	}

	for(x = 0; x < (NOBJS * 2 + 1); x++) {
		board[obj[x].y][obj[x].x] = obj[x].type;
	}

	for(x = 0; x < WIDTH; x++) {
		for(y = 0; y < HEIGHT; y++) {
			printf("%c%s", board[x][y], y == (HEIGHT - 1) ? "\n" : "");
		}
	}

	return;
}

int main(int argc, char *argv[])
{
	init();

	output();

	while(!stop) {
		input();
		process();
		output();
	}

	printf("Done!\n");

	return(0);
}
