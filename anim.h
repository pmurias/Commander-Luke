#ifndef __ANIM_H__
#define __ANIM_H__

#include "blit.h"

typedef struct
{
	Sprite **frames;
	int num_frames;
	float delay;
} AnimData;

AnimData *new_anim(void);
void anim_add_frame(AnimData *anim, char *frame);
void anim_add_frame_range(AnimData *namePrefix, char *frame, int start, int end);
void anim_blit_frame(AnimData *anim, int x, int y, float time);
void anim_free(AnimData **anim);

#endif // __ANIM_H__