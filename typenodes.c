
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

// struct list_head global_vars;
static LIST_HEAD(global_vars);


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

	struct variable *gvar;
	gvar = malloc(sizeof(struct variable));
	INIT_LIST_HEAD(&gvar->tnode.list);
	pr("VAR NAME : %s\n", 0, dwarf_diename(die));
	if (resolve_type_node(
				dwarf_formref_die(
					dwarf_attr_integrate(
						die,
						DW_AT_type,
						&attr),
					&type),
				&gvar->tnode.list) == 0)
	{
		if (list_empty(&gvar->tnode.list))
			pr("LIST NULL\n", 3);
		struct type_node *entry;
		pr("Start Listing resolved types\n", 3);
		list_for_each_entry(entry, &(gvar->tnode.list), list) {
			pr("%s %s \n", 6,
				dwarf_tag_string(dwarf_tag(&entry->die)),
				dwarf_diename(&entry->die));
		}
	}

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
		while (dwarf_nextcu (dbg, off, &off, &hsize, &abbrev, &addresssize,
					&offsetsize) == 0)
		{
			printf ("New CU: off = %llu, hsize = %zu, ab = %llu, as = %" PRIu8
					", os = %" PRIu8 "\n",
					(unsigned long long int) old_off, hsize,
					(unsigned long long int) abbrev, addresssize,
					offsetsize);

			Dwarf_Die die;
			if (dwarf_offdie(dbg, old_off + hsize, &die) != NULL)
				handle_die(dbg, &die, 1);

			old_off = off;
		}

		dwarf_end (dbg);
		close (fd);
	}
	return 0;
}
