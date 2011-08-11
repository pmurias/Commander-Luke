#ifndef __BLIT_H__
#define __BLIT_H__

#include <stdint.h>
#include "texture.h"

typedef struct
{
	float u;
	float v;
	float w;
	float h;
	int width;
	int height;
	Texture *texture;
} Sprite;

void blit_startup(void);
Sprite *blit_load_sprite(char *texname);
int blit_load_spritesheet(char *texname, char *mapname);
int blit_load_spritesheet_split(char *texname, char *mapname);
Sprite *blit_get_sprite(char *name);
void blit_sprite(Sprite *spr, int x, int y);
void blit_sprite_scaled(Sprite *spr, int x, int y, float s);
void blit_line(int x0, int y0, int x1, int y1, uint32_t color);

#endif // __BLIT_H__