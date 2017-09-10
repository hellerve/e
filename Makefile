TARGET=e
BUILDDIR=bin/
STXDIR=$(HOME)/.estx
ERC=$(HOME)/.erc
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
LUA_FLAGS=
LUA_OPT=generic
override CFLAGS+=-Werror -Wall -g -fPIC -O2 -DNDEBUG -ftrapv -Wfloat-equal -Wundef -Wwrite-strings -Wuninitialized -pedantic -std=c11

OS := $(shell uname)
DE := $(shell echo $(DESKTOP_SESSION))

ifeq ($(OS),$(filter $(OS), FreeBSD OpenBSD NetBSD))
CADDFLAG += -lexecinfo
endif

ifeq ($(DE),gnome)
CADDFLAG += `pkg-config --libs --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
DE_FLAG := -Dgnome
endif

all: main.c syntax
	$(info DE is $(DE))
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -DSTXDIR=\"$(STXDIR)\" -o $(BUILDDIR)$(TARGET) $(CFLAGS) $(CADDFLAG) $(DE_FLAG)

lua:
	cd ./vendor/lua-5.3.4/src && make clean && make $(LUA_OPT)
	make all CFLAGS+="-DWITH_LUA -L./vendor/lua-5.3.4/src -llua $(LUA_FLAGS)"

syntax:
	mkdir -p $(STXDIR)
	cp stx/* $(STXDIR)

install: all
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

install_lua:
	cd ./vendor/lua-5.3.4/src && make clean && make $(LUA_OPT) $(LUA_FLAGS)
	make install CFLAGS+="-DWITH_LUA -L./vendor/lua-5.3.4/src -llua -DERC=\"$(ERC)\" $(LUA_FLAGS)"
	touch ~/.erc

uninstall:
	rm -rf $(PREFIX)$(TARGET)
