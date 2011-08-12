#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "font.h"
#include "str.h"
#include "blit.h"

typedef struct
{
	Sprite *sprite;
	int x;
	int y;
	int width;
	int height;
	int xoff;
	int yoff;
	int xadvance;
} GlyphInfo;

typedef struct
{
	HashMap *font_map;
	Str *key_str;
} _InternalState;
_InternalState is;

//-----------------------------------------------------------------------------
void font_startup(void)
{
	is.font_map = new_hashmap(sizeof(Font*));
	is.key_str = new_str();
}

//-----------------------------------------------------------------------------
Font *new_font(void)
{
	Font *fnt = malloc(sizeof(Font));
	fnt->glyph_map = new_hashmap(sizeof(GlyphInfo));
	return fnt;
}

//-----------------------------------------------------------------------------
Font *font_load(char *texfile, char *fntfile)
{	
	printf("Loading font '%s'... ", fntfile);	
	Font *fnt = new_font();
	
	Texture *tex = texture_from_file(texfile);
	
	FILE *fp;
	if ((fp = fopen(fntfile, "rt")) == NULL) {
		printf("error: cannot open font file '%s'\n", fntfile);		
		return NULL;
	}
		
	char namebuf[255];
	Str *str = new_str();
					
	fscanf(fp, "font %s\n", namebuf);
	str_set(str, namebuf);
	hashmap_ins(is.font_map, str, &fnt);
	
	fscanf(fp, "size %d\n", &fnt->size);
	while (!feof(fp)) {
		int id;
		GlyphInfo glyph;		
		fscanf(fp, "char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d %*s %*s\n",
			&id, &glyph.x, &glyph.y, &glyph.width, &glyph.height, &glyph.xoff, &glyph.yoff, &glyph.xadvance);		
		
		glyph.sprite = malloc(sizeof(Sprite));
		glyph.sprite->texture = tex;		
		glyph.sprite->u = (float)glyph.x / tex->width;
		glyph.sprite->v = (float)glyph.y / tex->height;
		glyph.sprite->width = glyph.width;
		glyph.sprite->height = glyph.height;
		glyph.sprite->w = (float)glyph.width / tex->width;
		glyph.sprite->h = (float)glyph.height / tex->height;
		
		glyph.sprite->r = 1;
		glyph.sprite->g = 1;
		glyph.sprite->b = 1;
		
		namebuf[0] = id;
		namebuf[1] = 0;
		str_set(str, namebuf);
		hashmap_ins(fnt->glyph_map, str, &glyph);
	}	
	str_free(str);
	fclose(fp);
	printf("OK\n");
	return fnt;
}

//-----------------------------------------------------------------------------
Font *font_get(char *name)
{
	str_set(is.key_str, name);
	Font **fptr = hashmap_find(is.font_map, is.key_str);
	if (fptr) {
		return *fptr;
	} else {
		return NULL;
	}
}

//-----------------------------------------------------------------------------
void font_print(Font *fnt, int x, int y, float s, char *fmt, ...)
{	
	static char text[8196];
	va_list args;
	va_start(args, fmt);
	vsprintf(text, fmt, args);
	va_end(args);
	
	int cx = x;
	int cy = y;
	
	char namebuf[2];
	Str *str = new_str();
	for (int i=0; i<strlen(text); i++) {
		if (text[i] == '\n') {
			cx = x;
			cy += s * fnt->size;
			continue;
		}
		namebuf[0] = text[i];
		namebuf[1] = 0;
		str_set(str, namebuf);
		GlyphInfo *g = hashmap_find(fnt->glyph_map, str);
		if (g) {			
			blit_sprite_scaled(g->sprite, cx + round(s * g->xoff), cy + round(s * g->yoff), s);
			cx += s * g->xadvance;
		}
	}
	str_free(str);
}