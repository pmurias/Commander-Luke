#ifndef __BLIT_H__
#define __BLIT_H__

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
int blit_load_spritesheet(char *texname, char *mapname);
Sprite *blit_get_sprite(char *name);
void blit_sprite(Sprite *spr, int x, int y);

#endif // __BLIT_H__