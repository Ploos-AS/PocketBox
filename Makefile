CC ?= cc
CFLAGS ?= -Os -std=c11 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
all: pocketbox
pocketbox: src/main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDFLAGS)
test: pocketbox
	python3 tests/smoke.py
clean:
	rm -f pocketbox
.PHONY: all test clean
