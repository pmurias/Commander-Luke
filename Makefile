CC=gcc
CFLAGS=-c -std=gnu99 -Wall -O2 -Wfatal-errors

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
	socket.c
OBJECTS=$(SOURCES:.c=.o)

Release: $(OBJECTS)
	$(CC) $(OBJECTS) $(CLIBS) -o luke

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) luke like.exe
