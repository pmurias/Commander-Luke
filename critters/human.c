#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "iso.h"
#include "critter.h"

#define CRI_IDLE 0
#define CRI_RUNNING 1

typedef struct {
	CRITTER_BASE;
	float x;
	float y;

	float velocity;

	float face_x;
	float face_y;

	float anim_time;

	int state;

	float move_x;
	float move_y;

	float hp;
} Human;

//-----------------------------------------------------------------------------
static void tick(Critter * c)
{

	Human *cri = (Human *) c;

	if (cri->hp <= 0) {
		return;
	}

	cri->x += cri->face_x * cri->velocity;
	cri->y += cri->face_y * cri->velocity;
	cri->anim_time += 1.0 / 30.0f;

	if (cri->state == CRI_RUNNING) {
		cri->velocity = 0.12;
		cri->face_x = cri->move_x - cri->x;
		cri->face_y = cri->move_y - cri->y;
		float len = sqrt(cri->face_x * cri->face_x + cri->face_y * cri->face_y);
		if (len < 0.12) {
			cri->state = CRI_IDLE;
		}
		cri->face_x /= len;
		cri->face_y /= len;
	}
	if (cri->state == CRI_IDLE) {
		cri->velocity = 0;
	}
}

//-----------------------------------------------------------------------------
static void draw(Critter * c, float time_delta)
{
	Human *cri = (Human *) c;
	IsoAnim *anim = NULL;
	if (cri->hp <= 0) {
		Sprite *s = blit_get_sprite("Blurred_001");
		float wx, wy;
		iso_world2screen(cri->x, cri->y, &wx, &wy);
		blit_sprite(s, wx - 50, wy - 50);
		return;
		return;
	}
	if (cri->state == CRI_IDLE) {
		anim = isoanim_get("Nolty.Idle");
	} else if (cri->state == CRI_RUNNING) {
		anim = isoanim_get("Nolty.Running");
	}

	isoanim_blit_frame(anim, cri->x, cri->y, cri->anim_time, cri->face_x, cri->face_y);
}

//-----------------------------------------------------------------------------
static void think(Critter * critter)
{
}

//-----------------------------------------------------------------------------
static void damage(Critter * c, float amount)
{
	Human *cri = (Human *) c;
	cri->hp -= amount;
}

//-----------------------------------------------------------------------------
static void order(Critter * c, Netcmd * command)
{
	Human *cri = (Human *) c;
	switch (command->header.type) {
	case NETCMD_MOVECRITTER:{
			Netcmd_MoveCritter *move = (Netcmd_MoveCritter *) command;
			cri->state = CRI_RUNNING;
			cri->move_x = move->move_x;
			cri->move_y = move->move_y;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
static void get_viewpoint(Critter * c, float *x, float *y)
{
	Human *cri = (Human *) c;
	*x = cri->x;
	*y = cri->y;
}

//-----------------------------------------------------------------------------
float get_hp(Critter * c)
{
	Human *cri = (Human *) c;
        return cri->hp;
}

//-----------------------------------------------------------------------------
static CritterVTable *vtable;
void human_init_vtable()
{
	vtable = malloc(sizeof(CritterVTable));
	vtable->tick = tick;
	vtable->think = think;
	vtable->order = order;
	vtable->draw = draw;
	vtable->damage = damage;
	vtable->get_viewpoint = get_viewpoint;
	vtable->get_hp = get_hp;
}

//-----------------------------------------------------------------------------
Critter *new_human(float x, float y)
{
	Human *h = (Human *) malloc(sizeof(Human));
	h->vtable = vtable;
	h->x = x;
	h->y = y;
	h->velocity = 0;
	h->face_x = 0;
	h->face_y = 1;
	h->state = CRI_IDLE;
	h->anim_time = 0;
	h->hp = 100;
	return (Critter *) h;
}
