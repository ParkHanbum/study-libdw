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

int uftrace_type_string(struct global_var *gvar, char** result)
{
	if (list_empty(&gvar->tnode.list)) {
		pr("LIST NULL\n", 3);
		return -1;
	}

	struct type_node *entry;
	list_for_each_entry(entry, &(gvar->tnode.list), list) {
		pr("%s %s \n", 3,
		dwarf_tag_string(dwarf_tag(&entry->type.die)),
		dwarf_diename(&entry->type.die));
	}

}
