#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

//-----------------------------------------------------------------------------
Font *new_font(void)
{
	Font *fnt = malloc(sizeof(Font));
	fnt->glyph_map = new_hashmap(sizeof(GlyphInfo));
	return fnt;
}

//-----------------------------------------------------------------------------
int font_load(Font *fnt, char *texfile, char *fntfile)
{	
	Texture *tex = texture_from_file(texfile);
	
	FILE *fp;
	if ((fp = fopen(fntfile, "rt")) == NULL) {
		printf("error: cannot open font file '%s'\n", fntfile);		
		return 0;
	}
		
	char namebuf[2];
	Str *str = new_str();
		
	printf("Loading font '%s'\n", fntfile);
	
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
		
		namebuf[0] = id;
		namebuf[1] = 0;
		str_set(str, namebuf);
		hashmap_ins(fnt->glyph_map, str, &glyph);
	}	
	str_free(str);
	fclose(fp);
	return 1;
}

//-----------------------------------------------------------------------------
void font_print(Font *fnt, int x, int y, char *fmt, ...)
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
			cy += fnt->size;
			continue;
		}
		namebuf[0] = text[i];
		namebuf[1] = 0;
		str_set(str, namebuf);
		GlyphInfo *g = hashmap_find(fnt->glyph_map, str);
		if (g) {
			texture_bind(g->sprite->texture);
			blit_sprite(g->sprite, cx, cy + g->yoff);			
			cx += g->xadvance;
		}
	}
	str_free(str);
}

