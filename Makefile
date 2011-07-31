all:
	gcc -std=gnu99 -Wall main.c -lglfw -lopengl32 -lpng -lz -o luke
