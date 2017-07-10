TARGET=e
BUILDDIR=bin/
STXDIR=$(HOME)/.estx
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
override CFLAGS+=-Werror -Wall -g -fPIC -O2 -DNDEBUG -ftrapv -Wfloat-equal -Wundef -Wwrite-strings -Wuninitialized -pedantic -std=c11 $(LDFLAGS)
LDFLAGS=-L./vendor/lua-5.3.4/src -llua

all: main.c syntax
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -DSTXDIR=\"$(STXDIR)\" -o $(BUILDDIR)$(TARGET) $(CFLAGS)

lua:
	make all CFLAGS+=-DWITH_LUA

syntax:
	mkdir -p $(STXDIR)
	cp stx/* $(STXDIR)

install: all
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

install_lua:
	make install CFLAGS+=-DWITH_LUA

uninstall:
	rm -rf $(PREFIX)$(TARGET)
