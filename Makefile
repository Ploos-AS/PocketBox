CC ?= cc
CFLAGS ?= -Os -std=c11 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -D_GNU_SOURCE
LDFLAGS ?=
all: pocketbox
pocketbox: src/main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS)
test: pocketbox
	python3 tests/smoke.py
clean:
	rm -f pocketbox
.PHONY: all test clean
