CC=gcc
CFLAGS=-c -std=gnu99 -Wall -O2

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
	$(CC) -std=gnu99 -Wall -O2 socket.c test_socket.c $(CLIBS) -o test_socket

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) luke like.exe
