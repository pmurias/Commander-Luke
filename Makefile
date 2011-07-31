CC=gcc
CFLAGS=-c -std=gnu99 -Wall -O2

ifeq ($(OS),Windows_NT)
	CLIBS=-lglfw -lopengl32 -lpng -lz
else
	CLIBS=-lglfw -lpng -lz
endif

SOURCES= \
	main.c \
	texture.c \
	hashmap.c \
	str.c
OBJECTS=$(SOURCES:.c=.o)

Release: $(OBJECTS)
	$(CC) $(OBJECTS) $(CLIBS) -o luke

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) luke like.exe
