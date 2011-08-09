#ifndef __FONT_H__
#define __FONT_H__

#include "hashmap.h"

typedef struct
{	
	HashMap *glyph_map;
	int size;
} Font;

Font *new_font(void);
int font_load(Font *fnt, char *texfile, char *fntfile);
void font_print(Font *fnt, int x, int y, char *fmt, ...);

#endif
