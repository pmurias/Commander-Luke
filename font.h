#ifndef __FONT_H__
#define __FONT_H__

#include "hashmap.h"

typedef struct
{	
	HashMap *glyph_map;
	int size;
} Font;

void font_startup(void);
Font *new_font(void);
Font *font_load(char *texfile, char *fntfile);
Font *font_get(char *name);
void font_print(Font *fnt, int x, int y, float s, char *fmt, ...);
int font_str_width(Font *fnt, float s, char *fmt, ...);

#endif
