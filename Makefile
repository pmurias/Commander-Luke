all: luke
CC=gcc
CFLAGS=-c -std=gnu99 -I./core/ -Wall -O2 -Wfatal-errors -Werror -g

ifeq ($(OS),Windows_NT)
	CLIBS=-lglfw -lopengl32 -lpng -lz -lws2_32
else
	CLIBS=-lglfw -lpng -lz
endif

# core modules
SOURCES= \
	core/window.c \
	core/texture.c \
	core/hashmap.c \
	core/str.c \
	core/queue.c \
	core/array.c \
	core/socket.c \
	core/blit.c \
	core/anim.c \
	core/iso.c \
	core/font.c \
	core/single_player.c \
	core/tcp_client_state.c \
	core/tcp_server_state.c \
	core/rand.c

# game modules
SOURCES+= \
	main.c \
	camera.c \
	commands.c \
	ai.c \
	critter.c \
	spell.c \
	map_gen.c

	
# critters
SOURCES+= \
	critters/human.c\
	critters/blurred.c
	
# spells
SOURCES+= \
	spells/flare.c
	
OBJECTS=$(SOURCES:.c=.o)

bench: benchmark.o socket.o str.o
	$(CC) benchmark.o socket.o str.o $(CLIBS) -o benchmark

luke: $(OBJECTS)
	$(CC) $(OBJECTS) $(CLIBS) -o luke

%.o: %.c
	$(CC) $(CFLAGS) -I. $< -o $@

clean:
	rm -rf $(OBJECTS) luke like.exe

tests: tests/rand tests/ptrarray
tests/rand: tests/rand.c rand.o
	gcc -I. rand.o tests/rand.c -o tests/rand	
tests/ptrarray: tests/ptrarray.c array.o
	gcc -I. array.o tests/ptrarray.c -o tests/ptrarray
