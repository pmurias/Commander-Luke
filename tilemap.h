#ifndef __TILEMAP_H__
#define __TILEMAP_H__
typedef struct {
	int width;
	int height;
	int *tiles;
	int *wall_tiles;
} TileMap;
#endif
