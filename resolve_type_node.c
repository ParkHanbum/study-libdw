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

int resolve_type_node(Dwarf_Die *die, struct list_head *head)
{
	struct type_node *tnode;

	if (head == NULL)
		goto err;

	while(true) {
		pr("[Resolving] %s\n", 0, dwarf_diename(die));
		pr("[TAG] %s\n", 0, dwarf_tag_string(dwarf_tag(die)));
		tnode = malloc(sizeof(struct type_node));
		memcpy(&tnode->die, die, sizeof(Dwarf_Die));
		list_add(&tnode->list, head);
		if (!dwarf_hasattr(die, DW_AT_type)) {
			break;
		}

		Dwarf_Attribute attr_mem;
		Dwarf_Attribute *attr =
			dwarf_attr_integrate(die,
					DW_AT_type,
					&attr_mem);

		if (dwarf_formref_die(attr, die) == NULL)
			goto err;
	}
	return 0;
err:
	return -1;
}
