#include <dwarf.h>
#include <stdlib.h>
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

	// size of variable
	Dwarf_Word		size;
	// size of array (eg. upper - lower)
	Dwarf_Word		bound_size;
	// basetype mean "char" in "char *"
	Dwarf_Word		base_size;
	// encoding type. like signed
	Dwarf_Word		enctype;

	// absolute address of variable.
	uintptr_t		addr;

	// keep type_node for debugging purpose
	struct type_node	tnode;

	// contain member variable.
	struct list_head	member;
};

static inline struct variable *create_new_variable()
{
	struct variable *var = malloc(sizeof(struct variable));
	INIT_LIST_HEAD(&var->list);
	INIT_LIST_HEAD(&var->member);

	var->var_name = NULL;
	var->type_name = NULL;
	var->is_array = false;
	var->is_pointer = false;
	var->has_child = false;
	var->size = 0;
	var->bound_size = 0;
	var->base_size = 0;
	var->enctype = 0;
	var->addr = 0;
	return var;
}

static inline bool var_has_child(struct variable *var)
{
	return var->has_child;
}

#ifdef dev
# define pr(fmt, n, ...) 				\
({							\
 	printf("%20s:%-5d\t", __FILE__, __LINE__);		\
	printf("%*s  " fmt, n * 5, "", ## __VA_ARGS__);	\
})
# define prd(fmt, n, ...) 				\
({							\
 	printf("%20s:%-5d\t", __FILE__, __LINE__);		\
	printf("%*s  " fmt, n * 5, "", ## __VA_ARGS__);	\
})

# define pro(fmt, n, ...) 				\
({							\
	printf("%*s  " fmt, n * 5, "", ## __VA_ARGS__);	\
})

#else
# define pr(fmt, n, ...)
# define prd(fmt, n, ...)
# define pro(fmt, n, ...)
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

static const char *dwarf_encoding_string (unsigned int code)
{
	static const char *const known[] =
	{
#define DWARF_ONE_KNOWN_DW_ATE(NAME, CODE) [CODE] = #NAME,
		DWARF_ALL_KNOWN_DW_ATE
#undef DWARF_ONE_KNOWN_DW_ATE
	};

	if (code < sizeof (known) / sizeof (known[0]))
		return known[code];

	return NULL;
}

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

int resolve_variable(Dwarf_Die *die, struct variable *var);
int resolve_type_name(Dwarf_Die *die, struct variable *var);
int default_lower_bound(int lang, Dwarf_Sword *result);
