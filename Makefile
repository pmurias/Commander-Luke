all: luke
CC=gcc
CFLAGS=-c -std=gnu99 -Wall -O2 -Wfatal-errors -g

ifeq ($(OS),Windows_NT)
	CLIBS=-lglfw -lopengl32 -lpng -lz -lws2_32
else
	CLIBS=-lglfw -lpng -lz
endif

SOURCES= \
	main.c \
	texture.c \
	hashmap.c \
	str.c \
	socket.c \
	camera.c \
	single_player.c \
	blit.c \
	anim.c \
	iso.c
OBJECTS=$(SOURCES:.c=.o)

bench: benchmark.o socket.o str.o
	$(CC) benchmark.o socket.o str.o $(CLIBS) -o benchmark
	
luke: $(OBJECTS)
	$(CC) $(OBJECTS) $(CLIBS) -o luke

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) luke like.exe
