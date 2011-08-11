#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "hashmap.h"
#include "str.h"
#include "window.h"
#include "iso.h"

typedef struct InternalState {
	float dir_table[ISODIRECTIONS][2];
	Str *key_str;
	HashMap *isoanim_map;	
	
	int tile_width;
	int tile_height;
	
	float cam_x;
	float cam_y;
} InternalState;
static InternalState is;

//-----------------------------------------------------------------------------
void iso_startup(int tilew, int tileh)
{		
	float angleDist = M_PI*2.0 / ISODIRECTIONS;
	for (int i = 0; i < ISODIRECTIONS; i++) {
		float a = (i + 2) * angleDist;
		is.dir_table[i][0] = sin(a);
		is.dir_table[i][1] = cos(a);
	}
	is.isoanim_map = new_hashmap(sizeof(IsoAnim*));
	is.key_str = new_str();
	
	is.tile_width = tilew;
	is.tile_height = tileh;
	is.cam_x = 0;
	is.cam_y = 0;
}

//-----------------------------------------------------------------------------
void iso_set_cam(float x, float y)
{
	is.cam_x = x;
	is.cam_y = y;
}

//-----------------------------------------------------------------------------
int iso_tile_width(void)
{
	return is.tile_width;
}

//-----------------------------------------------------------------------------
int iso_tile_height(void)
{
	return is.tile_height;
}

//-----------------------------------------------------------------------------
int iso_get_dir(float x, float y)
{		
	float len = sqrt(x*x + y*y);
	if (len == 0)
		return 0;
	x /= len;
	y /= len;
	
	float clen = 1000;
	int cdir = 0;
	
	for (int i = 0; i < ISODIRECTIONS; i++) {
		float dx = x - is.dir_table[i][0];
		float dy = y - is.dir_table[i][1];
		float nsqlen = dx*dx + dy*dy;
		if (nsqlen < clen) {
			clen = nsqlen;
			cdir = i;
		}
	}	
	return cdir;  	
}

//-----------------------------------------------------------------------------
void iso_world2screen(float x, float y, float *ox, float *oy)
{
	x-= is.cam_x;
	y-= is.cam_y;
	*ox = (y - x) * is.tile_width/2 + window_width()/2;
	*oy = (x + y) * is.tile_height/2 + window_height()/2;
}

//-----------------------------------------------------------------------------
void iso_screen2world(float x, float y, float *ox, float *oy)
{
	*ox = (y-window_height()/2) / is.tile_height - (x-window_width()/2) / is.tile_width + is.cam_x;
	*oy = (x-window_width()/2) / is.tile_width + (y-window_height()/2) / is.tile_height + is.cam_y;
}

//-----------------------------------------------------------------------------
void iso_snap_screen2world(float x, float y, float *ox, float *oy)
{
	*ox = round((y-window_height()/2) / is.tile_height - (x-window_width()/2) / is.tile_width + is.cam_x);
	*oy = round((x-window_width()/2) / is.tile_width + (y-window_height()/2) / is.tile_height + is.cam_y);
}

//-----------------------------------------------------------------------------
IsoAnim *new_isoanim(void)
{
	IsoAnim *anim = malloc(sizeof(IsoAnim));	
	memset(anim->anims, 0, sizeof(AnimData*)*ISODIRECTIONS);
	return anim;
}

//-----------------------------------------------------------------------------
IsoAnim *isoanim_build(char *namePrefix, int len, float delay)
{	
	IsoAnim *anim = new_isoanim();
	str_set(is.key_str, namePrefix);
	hashmap_ins(is.isoanim_map, is.key_str, &anim);
	
	for (int i = 0; i < ISODIRECTIONS; i++) {		
		anim->anims[i] = new_anim();
		anim_add_frame_range(anim->anims[i], namePrefix, i*len+1, i*len+len);
		anim->anims[i]->delay = delay;
	}
	return anim;
}

//-----------------------------------------------------------------------------
IsoAnim *isoanim_get(char *name)
{
	str_set(is.key_str, name);
	IsoAnim **aptr = hashmap_find(is.isoanim_map, is.key_str);
	if (aptr) {
		return *aptr;
	} else {
		return NULL;
	}
}

//-----------------------------------------------------------------------------
void isoanim_blit_frame(IsoAnim *anim, int x, int y, float time, float dx, float dy)
{
	int dir = iso_get_dir(dx, dy);	
	anim_blit_frame(anim->anims[dir], x, y, time);
}

//-----------------------------------------------------------------------------
int isoanim_width(IsoAnim *anim)
{
	return anim->anims[0]->width;
}

//-----------------------------------------------------------------------------
int isoanim_height(IsoAnim *anim)
{
	return anim->anims[0]->height;
}
