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


static int get_type_size(Dwarf_Die *base)
{
	Dwarf_Word bits = 0;
	if (dwarf_hasattr(base, DW_AT_byte_size))
		bits = dwarf_bytesize(base);
	else if (dwarf_hasattr(base, DW_AT_bit_size))
		bits = dwarf_bitsize(base);

	return bits;
}

static int get_type_encode(Dwarf_Die *base)
{
	if (!dwarf_hasattr(base, DW_AT_encoding))
		return -1;

	Dwarf_Attribute attr_mem;
	Dwarf_Word encoding;
	if (dwarf_formudata (dwarf_attr_integrate (base, DW_AT_encoding,
					&attr_mem),
				&encoding) != 0)
		return -1;

	return encoding;
}

static int get_diename(Dwarf_Die *die, char *name)
{
	const char *temp;
	temp = dwarf_diename(die);
	if (temp == 0) {
		name = malloc(sizeof("[NONAME]"));
		strcpy(name, "[NONAME]");
		return 1;
	} else {
		name = malloc(strlen(temp));
		strcpy(name, temp);
	}

	return 0;
}

static int get_prefix_name(TYPE_PREFIX type_prefix, char **res)
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

static Dwarf_Word get_array_size(Dwarf_Die *array_die)
{
	Dwarf_Die *die;
	Dwarf_Die type_mem, aggregate_type_mem;
	Dwarf_Attribute attr_mem;
	Dwarf_Word count = 0;
	Dwarf_Word eltsize;

	die = malloc(sizeof(Dwarf_Die));

	pr("Enter get_array_size\n", 3);

	if (dwarf_child(array_die, die) == 0) {
		if (dwarf_tag(die) != DW_TAG_subrange_type) {
			// return 0 unknown array size;
			pr("NO subrange child\n", 3);
			return 0;
		}

		/* This has either DW_AT_count or DW_AT_upper_bound.  */
		if (dwarf_attr_integrate(die, DW_AT_count, &attr_mem) != NULL)
		{
			if (dwarf_formudata(&attr_mem, &count) != 0) {
				pr("reading attribute failed\n", 3);
				return 0;
			}
		}
		else
		{
			Dwarf_Sword upper;
			Dwarf_Sword lower;
			upper = 0, lower = 0;
			if (dwarf_formsdata(dwarf_attr_integrate
						(die, DW_AT_upper_bound,
						 &attr_mem), &upper) != 0)
			{
				pr("reading attribute failed\n", 3);
				return 0;
			}

			/* Having DW_AT_lower_bound is optional.  */
			if (dwarf_attr_integrate(die, DW_AT_lower_bound,
						&attr_mem) != NULL)
			{
				if (dwarf_formsdata(&attr_mem, &lower) != 0)
				{
					pr("reading attribute failed\n", 3);
					return 0;
				}
			}
			else
			{
				int lang = dwarf_srclang(&Die_CU);
				if (lang == -1 || default_lower_bound(lang, &lower) != 0)
				{
					pr("reading attribute[lower] failed\n", 3);
					return 0;
				}
			}
			if (lower > upper) {
				pr("LOWER bigger than UPPER\n", 3);
				return 0;
			}
			pr("LOWER : %ld\t UPPER : %ld\n", 3, lower, upper);

			if (lower == 0 && upper == 0)
				count = 0;

			else
				count = upper - lower + 1;
		}
		return count;
	}

	return 0;
}

static int get_type_name(Dwarf_Die *die, char **res)
{
	Dwarf_Attribute attr;
	bool is_signed;

	char assemble[128] = {0,};
	int tag = dwarf_tag(die);

	if (tag == DW_TAG_array_type) {
		Dwarf_Word val = get_array_size(die);
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

static int is_prefix_type(int tag)
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

static void set_type_prefix(int tag, TYPE_PREFIX *type_prefix)
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

static int resolve_type_node(Dwarf_Die *die, struct list_head *head)
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

		// if die has no DW_AT_type attribute then finish resolving.
		if (!dwarf_hasattr(die, DW_AT_type)) {
			break;
		}

		// to next die.
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

int resolve_variable(Dwarf_Die *die, struct variable *var)
{
	Dwarf_Attribute attr;
	Dwarf_Die type;
	const char *name;

	INIT_LIST_HEAD(&var->tnode.list);
	pr("VAR NAME : %s\n", 0, dwarf_diename(die));

	name = dwarf_diename(die);
	var->var_name = malloc(strlen(name));
	strcpy(var->var_name, name);

	if (resolve_type_node(
				dwarf_formref_die(
					dwarf_attr_integrate(
						die,
						DW_AT_type,
						&attr),
					&type),
				&var->tnode.list) != 0)
	{
		goto err;
	}

	if (list_empty(&var->tnode.list)) {
		pr("LIST NULL\n", 3);
		goto err;
	}

	pr("Start Listing resolved types\n", 3);

	TYPE_PREFIX type_prefix = TYPE_PREFIX_noprefix;
	char resolve[128] = {0,};
	char assemble[128] = {0,};
	char typed[128] = {0,};
	char *prefix = NULL;
	char *dtype = NULL;

	struct type_node *entry;

	// parsing data while iterating.
	list_for_each_entry_reverse(entry, &(var->tnode.list), list) {
		Dwarf_Die el = entry->die;
		int tag = dwarf_tag(&el);

		if (!is_prefix_type(tag)) {
			set_type_prefix(tag, &type_prefix);
		} else {
			get_type_name(&el, &dtype);
			if (type_prefix) {
				get_prefix_name(type_prefix, &prefix);
				type_prefix = TYPE_PREFIX_noprefix;
				assemble_type(typed, prefix, dtype, tag);
			} else {
				strcpy(typed, dtype);
			}

			sprintf(assemble, "%s %s", typed, resolve);
			strcpy(resolve, assemble);
		}

		if (tag == DW_TAG_array_type)
			var->is_array = true;
		else if (tag == DW_TAG_pointer_type)
			var->is_pointer = true;
		else if (tag == DW_TAG_structure_type) {
			var->base_size = get_type_size(&el);

			if (var->is_pointer)
				break;

			if (dwarf_haschildren(&el)) {
				var->has_child = true;
			}

			Dwarf_Die child;
			if (!var->has_child ||
				dwarf_child(&el, &child) < 0)
			{
				goto err;
			}

			if (dwarf_tag(&child) == DW_TAG_member) {
				do {
					pr("%s\n", 0, dwarf_diename(&child));
					struct variable *cvar = create_new_variable();
					if (resolve_variable(&child, cvar) != 0) {
						goto err;
					}
					list_add_tail(&cvar->list, &var->member);
				} while (dwarf_siblingof(&child, &child) == 0);
			}
		}

		else if (tag == DW_TAG_base_type) {
			var->base_size = get_type_size(&el);
			var->enctype = get_type_encode(&el);
		}

		pr("%s %s \n", 6,
			dwarf_tag_string(dwarf_tag(&entry->die)),
			dwarf_diename(&entry->die));
	}

	var->type_name = malloc(strlen(resolve));
	strcpy(var->type_name, resolve);

	// resolve finished.
	return 0;
err:
	pr("Error occured \n", 0);
	return -1;
}
