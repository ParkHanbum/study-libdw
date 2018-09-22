#include <dwarf.h>
#include <elfutils/libdw.h>
#include <elfutils/known-dwarf.h>
#include "list.h"

struct type {
	/* to present type structure. */
	struct list_head	*child;
	bool			has_child;
	Dwarf_Die		die;
};

struct type_node {
	struct list_head	list;
	struct type		type;
};

struct resolved_type {
	char			*var_name;
	char			*type_name;
	Dwarf_Word		bound_size;
	Dwarf_Word		base_size;
	bool			is_array;
	bool			is_pointer;
};

struct global_var {
	struct list_head	list;
	char			*var_name;
	char			*type_name;
	Dwarf_Word		size;
	struct resolved_type	*resolved_type;
	struct type_node	tnode;
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
