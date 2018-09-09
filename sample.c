#include <stdio.h>

struct global {
	int a;
	long b;
	char *c;
	char arr[20];
};

struct global gs;


void set_array(struct global *gs)
{
	char text[20] = {"Hello world"};
	for(int i = 0;i < 20;i++) {
		gs->arr[i] = text[i];
	}
}
void print_global(struct global gs)
{
	static int local_static = 100;
	printf("%x %d %lu %s %s\n", local_static, gs.a, gs.b, gs.c, gs.arr);
}

int main(int argc, char** argv)
{
	gs.a = 100;
	gs.b = 200;
	gs.c = "TEST";
	set_array(&gs);

	print_global(gs);
}
