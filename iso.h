#ifndef __ISO_H__
#define __ISO_H__

#include "anim.h"

#define ISODIRECTIONS 16

typedef struct
{		
	AnimData *anims[ISODIRECTIONS];
} IsoAnim;

void iso_startup();
int iso_get_dir(float x, float y);

IsoAnim *new_isoanim();
void isoanim_set(IsoAnim *anim, char *namePrefix, int len, float delay);
void isoanim_blit_frame(IsoAnim *anim, int x, int y, float time, float dx, float dy);

#endif