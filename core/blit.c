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
	int blend_mode;
} InternalState;
static InternalState is;

//-----------------------------------------------------------------------------
void blit_startup(void)
{
	is.sprites = malloc(sizeof(HashMap));
	hashmap_init(is.sprites, sizeof(Sprite*), 64);
	is.key_str = new_str();
	is.blend_mode = BLEND_ALPHA;
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
	
	sprite->r = 1;
	sprite->g = 1;
	sprite->b = 1;
	sprite->alpha = 1;
	
	sprite->center_x = sprite->width/2;
	sprite->center_y = sprite->height/2;
	sprite->angle = 0;
	
	sprite->blend_mode = BLEND_ALPHA;
	
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
		fscanf(fp, "%s = %f %f %d %d ", 
			namebuf, 
			&sprite->u, &sprite->v, 
			&sprite->width, &sprite->height);
		sprite->texture = tex;
		
		sprite->u /= tex->width;
		sprite->v /= tex->height;
		
		sprite->w = (float)sprite->width / tex->width;
		sprite->h = (float)sprite->height / tex->height;
		
		sprite->r = 1;
		sprite->g = 1;
		sprite->b = 1;
		sprite->alpha = 1;
		
		sprite->center_x = sprite->width/2;
		sprite->center_y = sprite->height/2;
		sprite->angle = 0;
		
		sprite->blend_mode = BLEND_ALPHA;
						
		str_set(is.key_str, namebuf);
		hashmap_ins(is.sprites, is.key_str, &sprite);
	}		
	
	fclose(fp);
	printf("OK\n");
	return 1;
}

//-----------------------------------------------------------------------------
int blit_load_spritesheet_split(char *texname, char *mapname)
{
	printf("Loading spritesheet '%s'... ", mapname);
	Image img;
	if (!image_load_from_file(&img, texname)) {
		printf("error: cannot open image.\n");
		return 0;
	}
	
	FILE *fp;
	
	if ((fp = fopen(mapname, "rt")) == NULL) {
		printf("error: cannot map file.\n");
		return 0;
	}
	
	char namebuf[255];	
		
	while (!feof(fp)) {			
		Sprite *sprite = malloc(sizeof(Sprite));
		fscanf(fp, "%s = %f %f %d %d ", 
			namebuf, 
			&sprite->u, &sprite->v,
			&sprite->width, &sprite->height);
		
		Texture *tex = texture_from_image_part(&img, sprite->u, sprite->v, sprite->width, sprite->height);
		sprite->texture = tex;
		
		sprite->u = 0;
		sprite->v = 0;		
		sprite->w = 1;
		sprite->h = 1;
		
		sprite->r = 1;
		sprite->g = 1;
		sprite->b = 1;
		sprite->alpha = 1;
		
		sprite->center_x = sprite->width/2;
		sprite->center_y = sprite->height/2;		
		sprite->angle = 0;
		
		sprite->blend_mode = BLEND_ALPHA;
						
		str_set(is.key_str, namebuf);
		hashmap_ins(is.sprites, is.key_str, &sprite);
	}	
	
	fclose(fp);
	image_free(&img);
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
	blit_set_blend_mode(spr->blend_mode);
	
	/* implementation is poor, but fast enough */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(x, y, 0);
	glRotatef(spr->angle, 0, 0, 1);
	
	glBegin(GL_QUADS);	
	glColor4f(spr->r, spr->g, spr->b, spr->alpha);
	glTexCoord2f(spr->u, spr->v);					
	glVertex2f(-spr->center_x*s, -spr->center_y*s);
	glTexCoord2f(spr->u, spr->v + spr->h );		
	glVertex2f(-spr->center_x*s, -spr->center_y*s + s*spr->height);
	glTexCoord2f(spr->u + spr->w, spr->v + spr->h);		
	glVertex2f(-spr->center_x*s + s*spr->width, -spr->center_y*s + s*spr->height);
	glTexCoord2f(spr->u + spr->w, spr->v);
	glVertex2f(-spr->center_x*s + s*spr->width, -spr->center_y*s);
	glEnd();	
	
	glPopMatrix();
}

//-----------------------------------------------------------------------------
void blit_line(int x0, int y0, int x1, int y1, uint32_t color)
{	
	glDisable(GL_TEXTURE_2D);
	
	glBegin(GL_LINES);
	glColor3f(((color >> 16) & 255)/255.0, ((color >> 8) & 255)/255.0, (color & 255)/255.0);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glEnd();
	
	glEnable(GL_TEXTURE_2D);
}


//-----------------------------------------------------------------------------
void blit_set_blend_mode(int blend_mode)
{
	if (is.blend_mode != blend_mode) {
		switch(blend_mode) {
			case BLEND_ALPHA:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BLEND_ADD:
				glBlendFunc(GL_ONE, GL_ONE);
				break;
		}
		is.blend_mode = blend_mode;
	}
}


