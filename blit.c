#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>

#include "blit.h"
#include "hashmap.h"

typedef struct
{
	HashMap *sprites;
	Str *key_str;	
} _InternalState;

static _InternalState is;

//-----------------------------------------------------------------------------
void blit_startup(void)
{
	is.sprites = malloc(sizeof(HashMap));
	hashmap_init(is.sprites, sizeof(Sprite*), 64);
	is.key_str = new_str();
}

//-----------------------------------------------------------------------------
Sprite *blit_load_sprite(char *texname)
{
	printf("Loading sprite '%s'... ", texname);
	Sprite *sprite = malloc(sizeof(Sprite));
	Texture *tex = texture_from_file(texname);
	sprite->texture = tex;
	sprite->u = 0;
	sprite->v = 0;
	sprite->width = tex->width;
	sprite->height = tex->height;
	sprite->w = 1;
	sprite->h = 1;
	
	str_set(is.key_str, texname);
	hashmap_ins(is.sprites, is.key_str, &sprite);
	
	printf("OK\n");
	return sprite;
}

//-----------------------------------------------------------------------------
int blit_load_spritesheet(char *texname, char *mapname)
{
	printf("Loading spritesheet '%s'... ", mapname);
	Texture *tex = texture_from_file(texname);
	
	FILE *fp;
	
	if ((fp = fopen(mapname, "rt")) == NULL) {
		printf("error: cannot map file.\n");
		return 0;
	}
	
	char namebuf[255];	
		
	while (!feof(fp)) {			
		Sprite *sprite = malloc(sizeof(Sprite));
		fscanf(fp, "%s = %f %f %d %d", 
			namebuf, 
			&sprite->u, &sprite->v, 
			&sprite->width, &sprite->height);
		sprite->texture = tex;
		
		sprite->u /= tex->width;
		sprite->v /= tex->height;
		
		sprite->w = (float)sprite->width / tex->width;
		sprite->h = (float)sprite->height / tex->height;
						
		str_set(is.key_str, namebuf);
		hashmap_ins(is.sprites, is.key_str, &sprite);
	}		
	
	fclose(fp);
	printf("OK\n");
	return 1;
}

//-----------------------------------------------------------------------------
Sprite *blit_get_sprite(char *name)
{	
	str_set(is.key_str, name);
	Sprite **sptr = hashmap_find(is.sprites, is.key_str);
	if (sptr) {
		return *sptr;
	} else {
		return NULL;
	}
}

//-----------------------------------------------------------------------------
void blit_sprite(Sprite *spr, int x, int y)
{	
	blit_sprite_scaled(spr, x, y, 1.0);
}

//-----------------------------------------------------------------------------
void blit_sprite_scaled(Sprite *spr, int x, int y, float s)
{		
	texture_bind(spr->texture);	
	
	/* implementation is poor, but fast enough */
	glBegin(GL_QUADS);
	glTexCoord2f(spr->u, spr->v);
	glVertex2f(x, y);
	glTexCoord2f(spr->u, spr->v + spr->h );
	glVertex2f(x, y + s * spr->height);
	glTexCoord2f(spr->u + spr->w, spr->v + spr->h);
	glVertex2f(x + s * spr->width, y + s * spr->height);
	glTexCoord2f(spr->u + spr->w, spr->v);
	glVertex2f(x + s * spr->width, y);
	glEnd();	
}
