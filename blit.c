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
void blit_startup()
{
	is.sprites = malloc(sizeof(HashMap));
	hashmap_init(is.sprites, sizeof(Sprite), 64);
	is.key_str = new_str();
}

//-----------------------------------------------------------------------------
int blit_load_spritesheet(char *texname, char *mapname)
{
	Texture *tex = texture_from_file(texname);
	
	FILE *fp;
	
	if ((fp = fopen(mapname, "rt")) == NULL) {
		printf("Error: cannot open spritesheet \"%s\".\n", mapname);
		return 0;
	}
	
	char namebuf[255];	
	
	printf("Loading spritesheet \"%s\"...\n", mapname);
	while (!feof(fp)) {			
		Sprite sprite;
		fscanf(fp, "%s = %f %f %d %d", 
			namebuf, 
			&sprite.u, &sprite.v, 
			&sprite.width, &sprite.height);
		sprite.texture = tex;
		
		sprite.u /= tex->width;
		sprite.v /= tex->height;
		
		sprite.w = (float)sprite.width / tex->width;
		sprite.h = (float)sprite.height / tex->height;
						
		str_set(is.key_str, namebuf);
		hashmap_ins(is.sprites, is.key_str, &sprite);
	}		
	
	fclose(fp);
	return 1;
}

//-----------------------------------------------------------------------------
Sprite *blit_get_sprite(char *name)
{	
	str_set(is.key_str, name);
	return hashmap_find(is.sprites, is.key_str);
}

//-----------------------------------------------------------------------------
void blit_sprite(Sprite *spr, int x, int y)
{	
	texture_bind(spr->texture);
	
	/* implementation is poor, but fast enough */
	glBegin(GL_QUADS);
	glTexCoord2f(spr->u, spr->v);
	glVertex2f(x, y);
	glTexCoord2f(spr->u, spr->v + spr->h );
	glVertex2f(x, y + spr->height);
	glTexCoord2f(spr->u + spr->w, spr->v + spr->h);
	glVertex2f(x + spr->width, y + spr->height);
	glTexCoord2f(spr->u + spr->w, spr->v);
	glVertex2f(x + spr->width, y);
	glEnd();	
}
