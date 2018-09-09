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

static int get_diename(Dwarf_Die *die, char **name)
{
	const char *temp;
	temp = dwarf_diename(die);
	if (temp == 0) {
		*name = malloc(sizeof("[NONAME]"));
		strcpy(*name, "[NONAME]");
		return 1;
	} else {
		*name = malloc(strlen(temp));
		strcpy(*name, temp);
	}

	return 0;
}

int dwarf_prefix_name(TYPE_PREFIX type_prefix, char **res)
{
	char *assemble = NULL;
	unsigned long alloc_size = 0;

	if (type_prefix & TYPE_PREFIX_typedef) {
		alloc_size += sizeof(TYPE_STR_typedef);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_typedef);
	}
	if (type_prefix & TYPE_PREFIX_const) {
		alloc_size += sizeof(TYPE_STR_const);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_const);
	}
	if (type_prefix & TYPE_PREFIX_volatile) {
		alloc_size += sizeof(TYPE_STR_volatile);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_volatile);
	}
	if (type_prefix & TYPE_PREFIX_restrict) {
		alloc_size += sizeof(TYPE_STR_restrict);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_restrict);
	}
	if (type_prefix & TYPE_PREFIX_atomic) {
		alloc_size += sizeof(TYPE_STR_atomic);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_atomic);
	}
	if (type_prefix & TYPE_PREFIX_immutable) {
		alloc_size += sizeof(TYPE_STR_immutable);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_immutable);
	}
	if (type_prefix & TYPE_PREFIX_packed) {
		alloc_size += sizeof(TYPE_STR_packed);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_packed);
	}
	if (type_prefix & TYPE_PREFIX_shared) {
		alloc_size += sizeof(TYPE_STR_shared);
		assemble = realloc(assemble, alloc_size);
		strcpy(assemble, TYPE_STR_shared);
	}

	*res = malloc(strlen(assemble));
	strcpy(*res, assemble);
	return 0;
}


Dwarf_Word dwarf_array_size(Dwarf_Die *array_die)
{
	Dwarf_Die *die;
	Dwarf_Word eltsize;
	Dwarf_Die type_mem, aggregate_type_mem;
	Dwarf_Word count;
	Dwarf_Attribute *attr_mem;

	if (dwarf_siblingof(array_die, die) == 0) {
		if (dwarf_tag(die) != DW_TAG_subrange_type) {
			// return 0 unknown array size;
			return 0;
		}

		/* This has either DW_AT_count or DW_AT_upper_bound.  */
		if (dwarf_attr_integrate(die, DW_AT_count, attr_mem) != NULL)
		{
			if (dwarf_formudata(attr_mem, &count) != 0)
				return -1;
		}
		else
		{
			Dwarf_Sword upper;
			Dwarf_Sword lower;
			if (dwarf_formsdata(dwarf_attr_integrate
						(die, DW_AT_upper_bound,
						 attr_mem), &upper) != 0)
				return -1;

			/* Having DW_AT_lower_bound is optional.  */
			if (dwarf_attr_integrate(die, DW_AT_lower_bound,
						attr_mem) != NULL)
			{
				if (dwarf_formsdata(attr_mem, &lower) != 0)
					return -1;
			}
			else
			{
				Dwarf_Die cu;
				// CUDIE = compile unit Die.first Die of current CU
				dwarf_offdie(dbg, dwarf_cuoffset(die), &cu);
				// Dwarf_Die cu = CUDIE (die->cu);
				int lang = dwarf_srclang(&cu);
				if (lang == -1 || dwarf_default_lower_bound(lang, &lower) != 0)
					return -1;
			}
			if (lower > upper) {
				pr("LOWER > UPPER\n", 5);
				return -1;
			}
			count = upper - lower + 1;
		}
		return count;
	}

	return -1;
}

int dwarf_type_name(Dwarf_Die *die, char **res)
{
	Dwarf_Attribute attr;
	bool is_signed;

	char assemble[128] = {0,};
	int tag = dwarf_tag(die);

	if (tag == DW_TAG_array_type) {
		Dwarf_Word val = dwarf_array_size(die);
		if (val > 0) {
			sprintf(assemble, "[%lu]", val);
		}
		else {
			sprintf(assemble, "[]");
		}
		*res = malloc(strlen(assemble));
		strcpy(*res, assemble);
	} else if (tag == DW_TAG_pointer_type) {
		*res = malloc(sizeof(TYPE_STR_pointer));
		strcpy(*res, TYPE_STR_pointer);
	} else if (tag == DW_TAG_structure_type) {
		const char *str = dwarf_diename(die);
		sprintf(assemble, "%s %s", TYPE_STR_structure, str);
		*res = malloc(strlen(assemble));
		strncpy(*res, assemble, strlen(assemble));
	} else if (tag == DW_TAG_base_type) {
		const char *str = dwarf_diename(die);
		*res = malloc(strlen(str));
		strncpy(*res, str, strlen(str));
	}
	pr("[RESOLVE] %d %s\n", 2, tag, *res);
	return 0;
}

int is_prefix_type(int tag)
{
	if ((tag == DW_TAG_typedef
		|| tag == DW_TAG_const_type
		|| tag == DW_TAG_volatile_type
		|| tag == DW_TAG_restrict_type
		|| tag == DW_TAG_atomic_type
		|| tag == DW_TAG_immutable_type
		|| tag == DW_TAG_packed_type
		|| tag == DW_TAG_shared_type))
		return 0;

	return 1;
}
void set_type_prefix(int tag, TYPE_PREFIX *type_prefix)
{
	switch(tag) {
		case DW_TAG_typedef:
			*type_prefix |= TYPE_PREFIX_typedef;
			break;
		case DW_TAG_const_type:
			*type_prefix |= TYPE_PREFIX_const;
			break;
		case DW_TAG_volatile_type:
			*type_prefix |= TYPE_PREFIX_volatile;
			break;
		case DW_TAG_restrict_type:
			*type_prefix |= TYPE_PREFIX_restrict;
			break;
		case DW_TAG_atomic_type:
			*type_prefix |= TYPE_PREFIX_atomic;
			break;
		case DW_TAG_immutable_type:
			*type_prefix |= TYPE_PREFIX_immutable;
			break;
		case DW_TAG_packed_type:
			*type_prefix |= TYPE_PREFIX_packed;
			break;
		case DW_TAG_shared_type:
			*type_prefix |= TYPE_PREFIX_shared;
			break;
	};
}

static int assemble_type(char *typed, char *prefix, char *type, int tag) {
	if (tag == DW_TAG_array_type) {
		sprintf(typed, "%s%s", prefix, type);
	}
	else {
		sprintf(typed, "%s %s", prefix, type);
	}

	return 0;
}

int resolve_type_name(Dwarf_Die *die, char **result)
{
	TYPE_PREFIX type_prefix = 0;

	if (die == NULL)
		return -1;

	char resolve[128] = {0,};
	char assemble[128] = {0,};
	char typed[128] = {0,};
	char *prefix = NULL;
	char *dtype = NULL;

	Dwarf_Attribute attr;
	Dwarf_Die type;

	struct global_var *gvar;
	gvar = malloc(sizeof(struct global_var));
	INIT_LIST_HEAD(&gvar->tnode.list);
	pr("[START RESOLVE TYPE]===========================\n", 0);
	pr("VAR NAME : %s\n", 0, dwarf_diename(die));

	if (resolve_type_node(die, &gvar->tnode.list) != 0
			|| list_empty(&gvar->tnode.list))
	{
		return -1;
	}

	struct type_node *entry;
	pr("Start Listing resolved types\n", 3);
	list_for_each_entry_reverse(entry, &(gvar->tnode.list), list) {
		Dwarf_Die curr_die = entry->type.die;

		pr("%s %s \n", 6,
				dwarf_tag_string(dwarf_tag(&curr_die)),
				dwarf_diename(&curr_die));

		int tag = dwarf_tag(&curr_die);

		pr("[TYPE] %s\n", 1,
				dwarf_tag_string(
					dwarf_tag(&curr_die)
					));

		if (!is_prefix_type(tag)) {
			set_type_prefix(tag, &type_prefix);
			pr("TAG : %s %d %llu\n", 1, dwarf_tag_string(tag), tag, type_prefix);
		} else {
			dwarf_type_name(&curr_die, &dtype);
			if (type_prefix) {
				dwarf_prefix_name(type_prefix, &prefix);
				pr("PREFIX TYPE: %s", 1, prefix);
				type_prefix = TYPE_PREFIX_noprefix;
				assemble_type(typed, prefix, dtype, tag);
			} else {
				strcpy(typed, dtype);
			}

			pr("TYPE %s\n", 1, typed);
			sprintf(assemble, "%s %s", typed, resolve);
			strcpy(resolve, assemble);
		}
	}
	pr("[END RESOLVE TYPE]===========================\n", 0);
	*result = malloc(strlen(resolve));
	strcpy(*result, resolve);

	pr("\n", 0);
	return 0;
}

