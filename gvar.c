#include <stdio.h>
#include "gvar.h"

int gvar_a;
long gvar_b;
char *gvar_c;
char gvar_d[128];
int gvar_f[64];
const char *gvar_g[10];

const char const * const gvar_expr[];

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
	printf("%x %d %lx %s %s\n", local_static, gs.a, gs.b, gs.c, gs.arr);
}

int main(int argc, char** argv)
{
	struct global lc;

	gvar_a = 100;
	gvar_b = 200;
	gvar_c = "HELLO WORLD\n";

	gs.a = 100;
	gs.b = 200;
	gs.c = "TEST";
	set_array(&gs);

	print_global(gs);

	lc.a = 300; lc.b = 400; lc.c = "TEST1";
	set_array(&lc);
	print_global(lc);

	static struct global lcs;
	lcs.a = 500; lcs.b = 600; lcs.c = "TEST2";
	set_array(&lcs);
	print_global(lcs);
}
