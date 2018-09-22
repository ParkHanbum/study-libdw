#include <dwarf.h>
#include <elfutils/libdw.h>
#include <elfutils/known-dwarf.h>
#include "list.h"

struct type_node {
	struct list_head	list;
	Dwarf_Die		die;
};

struct variable {
	struct list_head	list;
	char			*var_name;
	char			*type_name;

	bool			is_array;
	bool			is_pointer;
	bool			has_child;

	Dwarf_Word		size;
	Dwarf_Word		bound_size;
	Dwarf_Word		base_size;

	// absolute address of variable.
	uintptr_t		addr;

	// keep type_node for debugging purpose
	struct type_node	tnode;

	// contain member variable.
	struct list_head	member;
};

#ifdef dev
# define pr(fmt, n, ...) 				\
({							\
	printf("%*s " fmt, n * 5, "", ## __VA_ARGS__);	\
})
#else
# define pr(fmt, n, ...)
#endif

static const char *dwarf_tag_string (unsigned int tag)
{
	switch (tag)
	{
#define DWARF_ONE_KNOWN_DW_TAG(NAME, CODE) case CODE: return #NAME;
		DWARF_ALL_KNOWN_DW_TAG
#undef DWARF_ONE_KNOWN_DW_TAG
		default:
			return NULL;
	}
}

static const char *dwarf_attr_string (unsigned int attrnum)
{
	switch (attrnum)
	{
#define DWARF_ONE_KNOWN_DW_AT(NAME, CODE) case CODE: return #NAME;
		DWARF_ALL_KNOWN_DW_AT
#undef DWARF_ONE_KNOWN_DW_AT
		default:
			return NULL;
	}
}

/*
DW_TAG_typedef
DW_TAG_const_type
DW_TAG_volatile_type
DW_TAG_restrict_type
DW_TAG_atomic_type
DW_TAG_immutable_type
DW_TAG_packed_type
DW_TAG_shared_type
DW_TAG_array_type
*/


#define TYPE_PREFIX		unsigned long long
#define TYPE_PREFIX_noprefix	0
#define TYPE_PREFIX_typedef	1
#define TYPE_PREFIX_const	1<<1
#define TYPE_PREFIX_volatile	1<<2
#define TYPE_PREFIX_restrict	1<<3
#define TYPE_PREFIX_atomic	1<<4
#define TYPE_PREFIX_immutable	1<<5
#define TYPE_PREFIX_packed	1<<6
#define TYPE_PREFIX_shared	1<<7


// Prefix Strings
#define TYPE_STR_typedef	"typedef"
#define TYPE_STR_const		"const"
#define TYPE_STR_volatile	"volatile"
#define TYPE_STR_restrict	"restrict"
#define TYPE_STR_atomic		"atomic"
#define TYPE_STR_immutable	"immutable"
#define TYPE_STR_packed		"packed"
#define TYPE_STR_shared		"shared"

// Expression Strings
#define TYPE_STR_array		"[]"
#define TYPE_STR_pointer	"*"
#define TYPE_STR_structure	"struct"

// to keep current iterated CU
extern Dwarf *dbg;
extern Dwarf_Die Die_CU;

int resolve_type_node(Dwarf_Die *die, struct list_head *tnode);
int resolve_type_name(Dwarf_Die *die, char **result);
int default_lower_bound(int lang, Dwarf_Sword *result);
