
#include <inttypes.h>
#include <libelf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <elfutils/libdw.h>

#include "dwarf.h"
#include "dwarf_util.h"

// keep current CU information to global variable.
Dwarf *dbg;
Dwarf_Die Die_CU;

// declare list head as global.
static LIST_HEAD(gvar);

static void print_vars()
{
	struct variable *entry;
	char separator[90] = {0};
	memset(separator, '=', sizeof(separator)-1);

	if (list_empty(&gvar))
		pr("List is Empty", 0);

	printf("%s\n", separator);
	printf("%30s%20s%20s%12s\n", "[TYPE]", "[NAME]",
			"[ENCODING]", "[SIZE]");
	printf("%s\n", separator);

	list_for_each_entry(entry, &gvar, list) {
		printf("%30s", entry->type_name);
		printf("%20s", entry->var_name);
		printf("%20s", dwarf_encoding_string(entry->enctype));
		printf("%12ld\n", entry->base_size);
	}
}

void handle_die(Dwarf *dbg, Dwarf_Die *die, int n)
{
	Dwarf_Die child;
	unsigned int tag;

	tag = dwarf_tag(die);
	if (tag == DW_TAG_invalid)
		return;

	if (!dwarf_hasattr(die, DW_AT_external) || tag != DW_TAG_variable)
		goto next;

	Dwarf_Attribute attr;
	Dwarf_Die type;

	struct variable var;
	if (resolve_variable(die, &var) == 0)
		list_add_tail(&var.list, &gvar);
next:
	if (dwarf_haschildren(die) != 0 && dwarf_child(die, &child) == 0) {
		handle_die(dbg, &child, n + 1);
	}
	if (dwarf_siblingof(die, die) == 0) {
		handle_die(dbg, die, n);
	}
}

int main (int argc, char *argv[])
{
	int cnt;

	for (cnt = 1; cnt < argc; ++cnt)
	{
		int fd = open (argv[cnt], O_RDONLY);
		Dwarf *dbg;

		printf ("file: %s\n", basename (argv[cnt]));

		dbg = dwarf_begin (fd, DWARF_C_READ);
		if (dbg == NULL)
		{
			printf ("%s not usable\n", argv[cnt]);
			close (fd);
			continue;
		}

		Dwarf_Off off = 0;
		Dwarf_Off old_off = 0;
		size_t hsize;
		Dwarf_Off abbrev;
		uint8_t addresssize;
		uint8_t offsetsize;
		while (dwarf_nextcu(dbg, off, &off, &hsize, &abbrev,
					&addresssize, &offsetsize) == 0)
		{
			printf("New CU: off = %llu, hsize = %zu, ab = %llu"
					", as = %" PRIu8
					", os = %" PRIu8 "\n",
					(unsigned long long int) old_off,
					hsize,
					(unsigned long long int) abbrev,
					addresssize,
					offsetsize);

			Dwarf_Die die;
			if (dwarf_offdie(dbg, old_off + hsize, &die) != NULL) {
				// keep current CU Die to Die_CU
				memcpy(&Die_CU, &die, sizeof(Dwarf_Die));
				handle_die(dbg, &die, 1);
			}

			old_off = off;
		}

		dwarf_end (dbg);
		close (fd);
	}

	print_vars();
	return 0;
}
