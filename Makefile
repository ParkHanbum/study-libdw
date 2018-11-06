VERSION := 0.0.1

CFLAGS := -g -pg -Ddev=1 
LDFLAGS := -ldw -lelf

GVAR_SRCS := gvar.c
STUDY_SRCS := show-die-info.c

target: $(GVAR_SRCS)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

sample: sample.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

test_resolve: test_resolve_variables.c resolve_variables.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

show-die-info: show-die-info.c
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

all: target sample test_resolve show-die-info 

clean:
	rm $(wildcard.o) target sample test_resolve show-die-info

PHONY: all
