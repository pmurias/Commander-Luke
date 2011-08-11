#include <stdio.h>
#include <math.h>

#include "iso.h"
#include "critter.h"

void critter_tick(Critter *cri)
{
	cri->x += cri->face_x * cri->velocity;
	cri->y += cri->face_y * cri->velocity;
	cri->anim_time += 1.0/30.0f;
	
	if (cri->state == CRI_RUNNING) {
		cri->velocity = 0.12;		
		cri->face_x = cri->move_x - cri->x;
		cri->face_y = cri->move_y - cri->y;
		float len = sqrt(cri->face_x*cri->face_x + cri->face_y*cri->face_y);
		if (len < 0.1) {
			cri->state = CRI_IDLE;
		} 
		cri->face_x /= len;
		cri->face_y /= len;		
	}
	if (cri->state == CRI_IDLE) {
		cri->velocity = 0;
	}
}

void critter_draw(Critter *cri)
{
	float wx, wy;
	iso_world2screen(cri->x, cri->y, &wx, &wy);	
	IsoAnim *anim = NULL;
	if (cri->state == CRI_IDLE) {
		anim = isoanim_get("Nolty.Idle");			
	} else if (cri->state == CRI_RUNNING) {
		anim = isoanim_get("Nolty.Running");
	}
	wx -= isoanim_width(anim)/2;
	wy -= isoanim_height(anim) - isoanim_width(anim)/4;	
	isoanim_blit_frame(anim, wx, wy, cri->anim_time, cri->face_x, cri->face_y);
}

