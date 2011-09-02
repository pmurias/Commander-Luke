#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>

#include "hashmap.h"
#include "str.h"
#include "array.h"
#include "window.h"
#include "iso.h"

typedef struct
{
	Sprite *sprite;
	float x;
	float y;
} IsoZBatchElem;

typedef struct InternalState {
	float dir_table[ISODIRECTIONS][2];
	Str *key_str;
	HashMap *isoanim_map;	
	
	int tile_width;
	int tile_height;
	
	float cam_x;
	float cam_y;
	
	float ambient_r;
	float ambient_g;
	float ambient_b;
	PtrArray *lights;	
	Array *zbatch;
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
	is.lights = new_ptrarray();
	
	is.tile_width = tilew;
	is.tile_height = tileh;
	is.cam_x = 0;
	is.cam_y = 0;	
	
	is.zbatch = new_array(sizeof(IsoZBatchElem));
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
	*ox = round((y - x) * is.tile_width/2 + window_width()/2);
	*oy = round((x + y) * is.tile_height/2 + window_height()/2);
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
void iso_blit_tile(Texture *tex, int x, int y)
{
	float sx, sy;
	iso_world2screen(x, y, &sx, &sy);	
	texture_bind(tex);
	
	float r, g, b;
	
	glBegin(GL_QUADS);
		
	iso_illuminate(x+0.5, y-0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0, 0.5);
	glVertex2f(sx - is.tile_width/2, sy);
	
	iso_illuminate(x, y-0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.25, 0.25);
	glVertex2f(sx - is.tile_width/4, sy - is.tile_height/4);
	
	iso_illuminate(x, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.5, 0.5);
	glVertex2f(sx, sy);
	
	iso_illuminate(x+0.5, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.25, 0.75);
	glVertex2f(sx - is.tile_width/4, sy + is.tile_height/4);
	
	//----------
	
	iso_illuminate(x, y-0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.25, 0.25);
	glVertex2f(sx - is.tile_width/4, sy - is.tile_height/4);
	
	iso_illuminate(x-0.5, y-0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.5, 0);
	glVertex2f(sx, sy - is.tile_height/2);
	
	iso_illuminate(x-0.5, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.75, 0.25);
	glVertex2f(sx + is.tile_width/4, sy - is.tile_height/4);
	
	iso_illuminate(x, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.5, 0.5);
	glVertex2f(sx, sy);
	
	//----------
	
	iso_illuminate(x, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.5, 0.5);
	glVertex2f(sx, sy);
	
	iso_illuminate(x-0.5, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.75, 0.25);
	glVertex2f(sx + is.tile_width/4, sy - is.tile_height/4);	
	
	iso_illuminate(x-0.5, y+0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(1, 0.5);
	glVertex2f(sx + is.tile_width/2, sy);
	
	iso_illuminate(x, y+0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.75, 0.75);
	glVertex2f(sx + is.tile_width/4, sy + is.tile_height/4);
		
	//----------
	
	iso_illuminate(x+0.5, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.25, 0.75);
	glVertex2f(sx - is.tile_width/4, sy + is.tile_height/4);
	
	iso_illuminate(x, y, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.5, 0.5);
	glVertex2f(sx, sy);
	
	iso_illuminate(x, y+0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.75, 0.75);
	glVertex2f(sx + is.tile_width/4, sy + is.tile_height/4);
	
	iso_illuminate(x+0.5, y+0.5, &r, &g, &b);
	glColor3f(r, g, b);
	glTexCoord2f(0.5, 1);
	glVertex2f(sx, sy + is.tile_height/2);
	
	glEnd();
}

//-----------------------------------------------------------------------------
IsoLight *new_isolight(void)
{
	IsoLight *li = malloc(sizeof(IsoLight));
	ptrarray_add(is.lights, li);	
	return li;
}

//-----------------------------------------------------------------------------
void free_isolight(IsoLight **light)
{		
	if (ptrarray_free(is.lights, *light)) {
		*light = NULL;
	}	
}

//-----------------------------------------------------------------------------
void iso_illuminate(float x, float y, float *r, float *g, float *b)
{
	*r = is.ambient_r;
	*g = is.ambient_g;
	*b = is.ambient_b;
	float power = 0;
	for (int i = 0; i<is.lights->count; i++) {
		IsoLight *li = (IsoLight *)ptrarray(is.lights)[i];

		power = pow(li->range,2) * 0.1/(pow(x - li->x, 2)+pow(y - li->y, 2));
		power = (power < 1 ? power : 1);
		*r += li->r * power;
		*g += li->g * power;
		*b += li->b * power;
	}
}

//-----------------------------------------------------------------------------
void iso_set_ambient(float r, float g, float b)
{
	is.ambient_r = r;
	is.ambient_g = g;
	is.ambient_b = b;
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
void isoanim_blit_frame(IsoAnim *anim, float x, float y, float time, float dirx, float diry)
{
	float wx, wy;
	iso_world2screen(x, y, &wx, &wy);
	
	int dir = iso_get_dir(dirx, diry);	
	Sprite *frame = anim_get_frame(anim->anims[dir], time);
	iso_illuminate(x, y, &frame->r, &frame->g, &frame->b);
	anim_blit_frame(anim->anims[dir], wx, wy, time);
}

//-----------------------------------------------------------------------------
void isoanim_set_center(IsoAnim *anim, int cen_x, int inv_cen_y)
{
	for (int i=0; i<ISODIRECTIONS; i++) {
		Sprite *frame = anim->anims[i]->frames[0];
		anim_set_center(anim->anims[i], cen_x, frame->height - inv_cen_y);
	}
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

//-----------------------------------------------------------------------------
void isozbatch_add_sprite(Sprite *s, float x, float y)
{
	IsoZBatchElem elem;
	elem.sprite = s;
	elem.x = x;
	elem.y = y;
	array_add(is.zbatch, &elem);
}

//-----------------------------------------------------------------------------
void isozbatch_add_frame(IsoAnim *anim, float x, float y, float time, float dirx, float diry)
{
	int dir = iso_get_dir(dirx, diry);	
	Sprite *frame = anim_get_frame(anim->anims[dir], time);
	isozbatch_add_sprite(frame, x, y);
}

//-----------------------------------------------------------------------------
static int isozbatchelem_compare(const void *aptr, const void *bptr)
{
	IsoZBatchElem *a = (IsoZBatchElem*)aptr;
	IsoZBatchElem *b = (IsoZBatchElem*)bptr;
	return ((a->x + a->y) - (b->x + b->y)) > 0;
}

//-----------------------------------------------------------------------------
void isozbatch_draw(void)
{
	float wx, wy;
	IsoZBatchElem *e;
	
	array_sort(is.zbatch, isozbatchelem_compare);
	for (int i = 0; i<is.zbatch->count; i++) {
		e = array_get(is.zbatch, i);
		iso_world2screen(e->x, e->y, &wx, &wy);	
		iso_illuminate(e->x, e->y, &e->sprite->r, &e->sprite->g, &e->sprite->b);
		blit_sprite(e->sprite, wx, wy);
	}
	array_clear(is.zbatch);
}

