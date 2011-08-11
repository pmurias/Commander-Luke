#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "blit.h"
#include "anim.h"

//-----------------------------------------------------------------------------
AnimData *new_anim(void)
{
	AnimData *anim = malloc(sizeof(AnimData));
	anim->frames = malloc(sizeof(Sprite*));
	anim->frames[0] = NULL;
	anim->delay = 0.0;
	anim->num_frames = 0;
	return anim;
}

//-----------------------------------------------------------------------------
void anim_add_frame(AnimData *anim, char *frame)
{
	anim->num_frames += 1;
	anim->frames = realloc(anim->frames, anim->num_frames *sizeof(Sprite*));
	anim->frames[anim->num_frames-1] = blit_get_sprite(frame);
	
	anim->width = anim->frames[anim->num_frames-1]->width;
	anim->height = anim->frames[anim->num_frames-1]->height;
}

//-----------------------------------------------------------------------------
void anim_add_frame_range(AnimData *anim, char *namePrefix, int start, int end)
{
	char namebuf[255];
	anim->frames = realloc(anim->frames, (anim->num_frames + (end - start + 1)) * sizeof(Sprite*));
	for (int i = start; i <= end; i++) {
		sprintf(namebuf, "%s%03d", namePrefix, i);		
		anim->frames[anim->num_frames] = blit_get_sprite(namebuf);
		anim->num_frames += 1;
	}
	anim->width = anim->frames[0]->width;
	anim->height = anim->frames[0]->height;
}

//-----------------------------------------------------------------------------
void anim_blit_frame(AnimData *anim, int x, int y, float time)
{	
	blit_sprite(anim->frames[(int)(round(time / anim->delay)) % anim->num_frames], x, y);
}

//-----------------------------------------------------------------------------
void anim_free(AnimData **anim)
{
	free((*anim)->frames);
	free(*anim);
	*anim = NULL;
}
