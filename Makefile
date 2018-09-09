VERSION := 0.0.1

CFLAGS := -g -pg -Ddev=1 
LDFLAGS := -ldw -lelf

GVAR_SRCS := gvar.c
STUDY_SRCS := show-die-info.c
RESOLVE_TYPE_NAME := resolve_type_node.c resolve_type_name.c

target: $(GVAR_SRCS)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

sample: sample.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

varnames: varnames.c $(RESOLVE_TYPE_NAME)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

typenodes: typenodes.c resolve_type_node.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

all: target sample typenodes varnames

clean:
	rm varnames target sample typenodes $(wildcard.o)
