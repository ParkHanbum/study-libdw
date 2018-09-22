
#include <inttypes.h>
#include <libelf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "dwarf.h"
#include "dwarf_util.h"

// keep current CU information to global variable.
Dwarf *dbg;
Dwarf_Die Die_CU;

// struct list_head global_vars;
static LIST_HEAD(global_vars);

static void print_global_vars()
{
	struct global_var *gv;

	if (list_empty(&global_vars))
		pr("List is Empty", 0);

	list_for_each_entry(gv, &global_vars, list) {
		if (gv->type_name != NULL) {
			pr("%s", 0, gv->type_name);
		}
		if (gv->var_name != NULL) {
			pr("%s", 0, gv->var_name);
		}
		pr("%lu", 1, gv->size);
		pr("\n", 0);
	}
}

void handle(Dwarf *dbg, Dwarf_Die *die, int n)
{
	Dwarf_Die child;
	unsigned int tag;
	const char *str;
	char buf[30];
	const char *name;
	Dwarf_Off off;
	Dwarf_Off cuoff;
	size_t cnt;
	Dwarf_Addr addr;
	int i;

	tag = dwarf_tag (die);
	if (tag == DW_TAG_invalid)
		return;

	if (!dwarf_hasattr(die, DW_AT_external) || tag != DW_TAG_variable)
		goto out;

	Dwarf_Attribute attr;
	Dwarf_Die type;

	char *result = NULL;
	struct global_var *gvar;
	gvar = malloc(sizeof(struct global_var));

	if (resolve_type_name(dwarf_formref_die (dwarf_attr_integrate (die,
						DW_AT_type,
						&attr), &type),
				&result) == 0);
	{
		pr("[RESOLVED TYPE] %s\n", 1, result);
		gvar->type_name = result;
		name = dwarf_diename(die);
		Dwarf_Word val;
		gvar->var_name = malloc(strlen(name));
		strcpy(gvar->var_name, name);

		gvar->size = 0;
		Dwarf_Word size = 0;
		if (dwarf_aggregate_size(&type, &size) < 0)
			pr("%s no size : %s\n", n, name, dwarf_errmsg(-1));
		else
			pr("%s size %lu\n", n, name, size);
		gvar->size = size;

		pr("Attrs     :", n);
		for (cnt = 0; cnt < 0xffff; ++cnt)
			if (dwarf_hasattr (die, cnt))
				pr("%s", 0, dwarf_attr_string (cnt));
		puts ("");
		pr("[TEST] %s %s %lu\n", n, gvar->var_name, gvar->type_name, gvar->size);
		list_add_tail(&gvar->list, &global_vars);
		pr("[HANDLE FINISH] =======================\n", 0);

	}

out:

	if (dwarf_haschildren (die) != 0 && dwarf_child (die, &child) == 0) {
		// pr("[CHILD] \n", (n+1));
		handle (dbg, &child, n + 1);
	}
	if (dwarf_siblingof (die, die) == 0) {
		// pr("[SIBLING] \n", n);
		handle (dbg, die, n);
	}
}

int main (int argc, char *argv[])
{
	int cnt;

	// INIT_LIST_HEAD(&global_vars);

	for (cnt = 1; cnt < argc; ++cnt)
	{
		int fd = open (argv[cnt], O_RDONLY);
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
		while (dwarf_nextcu (dbg, off, &off, &hsize, &abbrev, &addresssize,
					&offsetsize) == 0)
		{
			printf ("New CU: off = %llu, hsize = %zu, ab = %llu, as = %" PRIu8
					", os = %" PRIu8 "\n",
					(unsigned long long int) old_off, hsize,
					(unsigned long long int) abbrev, addresssize,
					offsetsize);

			Dwarf_Die die;
			if (dwarf_offdie (dbg, old_off + hsize, &die) != NULL) {
				// keep CU Die
				memcpy(&Die_CU, &die, sizeof(Dwarf_Die));
				handle (dbg, &die, 1);
			}

			old_off = off;
		}

		dwarf_end (dbg);
		close (fd);
	}


	print_global_vars();
	return 0;
}
