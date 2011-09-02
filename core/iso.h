#ifndef __ISO_H__
#define __ISO_H__

#include "anim.h"

#define ISODIRECTIONS 16

typedef struct
{		
	AnimData *anims[ISODIRECTIONS];	
} IsoAnim;

typedef struct
{
	float x;
	float y;
	float range;
	float r;
	float g;
	float b;
} IsoLight;

void iso_startup(int tilew, int tileh);
void iso_set_cam(float x, float y);
int iso_get_dir(float x, float y);
int iso_tile_width(void);
int iso_tile_height(void);

void iso_world2screen(float x, float y, float *ox, float *oy);
void iso_screen2world(float x, float y, float *ox, float *oy);
void iso_snap_screen2world(float x, float y, float *ox, float *oy);
void iso_blit_tile(Texture *tex, int x, int y);

IsoLight *new_isolight(void);
void free_isolight(IsoLight **light);
void iso_illuminate(float x, float y, float *r, float *g, float *b);
void iso_set_ambient(float r, float g, float b);

IsoAnim *new_isoanim(void);
IsoAnim *isoanim_build(char *namePrefix, int len, float delay);
IsoAnim *isoanim_get(char *name);
void isoanim_blit_frame(IsoAnim *anim, float x, float y, float time, float dirx, float diry);
void isoanim_set_center(IsoAnim *anim, int cen_x, int inv_cen_y);
int isoanim_width(IsoAnim *anim);
int isoanim_height(IsoAnim *anim);

void isozbatch_add_sprite(Sprite *s, float x, float y);
void isozbatch_add_frame(IsoAnim *anim, float x, float y, float time, float dirx, float diry);
void isozbatch_draw(void);

#endif