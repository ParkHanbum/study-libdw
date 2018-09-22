VERSION := 0.0.1

CFLAGS := -g -Ddev=1 
LDFLAGS := -ldw -lelf

GVAR_SRCS := gvar.c
STUDY_SRCS := show-die-info.c
RESOLVE_TYPE_NAME := resolve_type_node.c resolve_type_name.c default_lower_bound.c

target: $(GVAR_SRCS)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

sample: sample.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

varnames: varnames.c $(RESOLVE_TYPE_NAME)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

typenodes: typenodes.c resolve_type_node.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

show-die-info: show-die-info.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

all: target sample typenodes varnames show-die-info

clean:
	rm varnames target sample typenodes $(wildcard.o)
