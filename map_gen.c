#include <stdlib.h>
#include <stdio.h>
#include "tilemap.h"
#include "rand.h"
#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))
void dig(TileMap* map,int x,int y) {
  	if (x < 0 || x >= map->width || y < 0 || y >= map->height) return;
	printf("%d %d\n",x,y);
	if (map->tiles[y*map->width+x] == 0) return;
  	map->tiles[y*map->width+x] = 0;
	for (int dx=-1;dx<2;dx++) {
		for (int dy=-1;dy<2;dy++) {
			if (rand_rand()%5 == 0) {
				dig(map,x+dx,y+dy);
			}
		}
	}
}
static void render(TileMap* map,char* filename) {
  	FILE* f = fopen(filename,"w");
	fprintf(f,"P1\n");
	fprintf(f,"%d %d\n",map->width,map->height);
	for (int y = 0; y < map->height; y++) {
		for (int x = 0; x < map->width; x++) {
			fprintf(f,"%d ",map->tiles[map->width*y+x] != 0);
		}
		fprintf(f,"\n");
	}
	fclose(f);
}

TileMap *overworld_gen(int w, int h)
{
	TileMap *map = malloc(sizeof(TileMap));
	map->tiles = NEWC(int, w * h);
	map->wall_tiles = NEWC(int, w * h);
	map->width = w;
	map->height = h;
	for (int i = 0; i < w * h; i++) {
		map->tiles[i] = 1+rand_rand()%4;
		map->wall_tiles[i] = 0;
	}

	int holes = 300;
	for (int i = 0; i < holes; i++) {
		dig(map,rand_rand()%w,rand_rand()%h);
	}

	for (int y = 1; y < map->height-1; y++) {
		for (int x = 1; x < map->width-1; x++) {
		    if (map->tiles[y*map->width+x] == 0) {
		    	if (map->tiles[y*map->width+x] == 0) {
				if (map->tiles[(y+1)*map->width+x]) map->wall_tiles[y*map->width+x] |= 2;
				if (map->tiles[(y)*map->width+x+1]) map->wall_tiles[y*map->width+x] |= 4;
				if (map->tiles[(y-1)*map->width+x]) map->wall_tiles[y*map->width+x] |= 8;
				if (map->tiles[(y)*map->width+x-1]) map->wall_tiles[y*map->width+x] |= 16;
			}
		    }
				
		}
	}

	render(map,"map.pbm");
	return map;
}

