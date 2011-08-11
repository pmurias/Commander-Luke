#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "iso.h"
#include "critter.h"

#define CRI_IDLE 0
#define CRI_RUNNING 1

typedef struct
{	
        CRITTER_BASE
	float x;
	float y;
	
	float velocity;
	
	float face_x;
	float face_y;
	
	float anim_time;
	
	int state;
	
	float move_x;
	float move_y;
} Human;

static void tick(Critter *c)
{
        Human *cri = (Human*) c;
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

static void draw(Critter *c,float time_delta)
{
        Human *cri = (Human*) c;
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
static void think(Critter *critter) {
}
static void order(Critter *c,Netcmd *command) {
    Human *cri = (Human*) c;
    switch (command->header.type) {
      case NETCMD_MOVECRITTER: {
        Netcmd_MoveCritter *move = (Netcmd_MoveCritter*) command;
	cri->state = CRI_RUNNING;
	cri->move_x = move->move_x;
	cri->move_y = move->move_y;
      break;
                               }
  }
}

static void get_viewpoint(Critter* c,float *x,float *y) {
    Human *cri = (Human*) c;
    *x = cri->x;
    *y = cri->y;
}

CritterVTable* vtable;
void human_init() {
  vtable = malloc(sizeof(CritterVTable));
  vtable->tick = tick;
  vtable->think = think;
  vtable->order = order;
  vtable->draw = draw;
  vtable->get_viewpoint = get_viewpoint;
}

Critter* human_new(float x,float y) {
    Human* h = (Human*) malloc(sizeof(Human));
    h->vtable = vtable;
    h->x = x;
    h->y = y;
    h->velocity = 0;
    h->face_x = 0;
    h->face_y = 1;
    h->state = CRI_IDLE;
    return (Critter*)h;
}
