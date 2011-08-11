all: luke
CC=gcc
CFLAGS=-c -std=gnu99 -Wall -O2 -Wfatal-errors -Werror -g

ifeq ($(OS),Windows_NT)
	CLIBS=-lglfw -lopengl32 -lpng -lz -lws2_32
else
	CLIBS=-lglfw -lpng -lz
endif

# core modules
SOURCES= \
	window.c \
	texture.c \
	hashmap.c \
	str.c \
	queue.c \
	socket.c \
	blit.c \
	anim.c \
	iso.c \
	font.c \
	single_player.c \
	tcp_client_state.c \
	tcp_server_state.c \
	rand.c \
	critters/human.c

# game modules
SOURCES+= \
	main.c \
	camera.c \
	commands.c \
	critter.c
	
OBJECTS=$(SOURCES:.c=.o)

bench: benchmark.o socket.o str.o
	$(CC) benchmark.o socket.o str.o $(CLIBS) -o benchmark
	
luke: $(OBJECTS)
	$(CC) $(OBJECTS) $(CLIBS) -o luke

%.o: %.c
	$(CC) $(CFLAGS) -I. $< -o $@

clean:
	rm -rf $(OBJECTS) luke like.exe

tests: tests/rand
tests/rand: tests/rand.c rand.o
	gcc -I. rand.o tests/rand.c -o tests/rand
