#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iso.h"

static float dir_table[ISODIRECTIONS][2];

//-----------------------------------------------------------------------------
void iso_startup(void)
{		
	float angleDist = M_PI*2.0 / ISODIRECTIONS;
	for (int i = 0; i < ISODIRECTIONS; i++) {
		float a = i * angleDist;
		dir_table[i][0] = -sin(a);
		dir_table[i][1] = cos(a);		
	}
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
		float dx = x - dir_table[i][0];
		float dy = y - dir_table[i][1];
		float nsqlen = dx*dx + dy*dy;
		if (nsqlen < clen) {
			clen = nsqlen;
			cdir = i;
		}
	}	
	return cdir;  	
}

//-----------------------------------------------------------------------------
IsoAnim *new_isoanim(void)
{
	IsoAnim *anim = malloc(sizeof(IsoAnim));	
	memset(anim->anims, 0, sizeof(AnimData*)*ISODIRECTIONS);
	return anim;
}

//-----------------------------------------------------------------------------
void isoanim_set(IsoAnim *anim, char *namePrefix, int len, float delay)
{	
	for (int i = 0; i < ISODIRECTIONS; i++) {		
		anim->anims[i] = new_anim();
		anim_add_frame_range(anim->anims[i], namePrefix, i*len+1, i*len+len);
		anim->anims[i]->delay = delay;
	}
}

//-----------------------------------------------------------------------------
void isoanim_blit_frame(IsoAnim *anim, int x, int y, float time, float dx, float dy)
{
	int dir = iso_get_dir(dx, dy);
	anim_blit_frame(anim->anims[dir], x, y, time);
}