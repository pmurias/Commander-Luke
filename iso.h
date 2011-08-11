#ifndef __ISO_H__
#define __ISO_H__

#include "anim.h"

#define ISODIRECTIONS 16

typedef struct
{		
	AnimData *anims[ISODIRECTIONS];	
} IsoAnim;

void iso_startup(int tilew, int tileh);
void iso_set_cam(float x, float y);
int iso_get_dir(float x, float y);
int iso_tile_width(void);
int iso_tile_height(void);

void iso_world2screen(float x, float y, float *ox, float *oy);
void iso_screen2world(float x, float y, float *ox, float *oy);
void iso_snap_screen2world(float x, float y, float *ox, float *oy);

IsoAnim *new_isoanim(void);
IsoAnim *isoanim_build(char *namePrefix, int len, float delay);
IsoAnim *isoanim_get(char *name);
void isoanim_blit_frame(IsoAnim *anim, int x, int y, float time, float dx, float dy);
int isoanim_width(IsoAnim *anim);
int isoanim_height(IsoAnim *anim);

#endif