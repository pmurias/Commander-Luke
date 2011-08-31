#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iso.h"
#include "serializers.h"
#include "critter.h"
#include "rand.h"
#include "ai.h"

#define CRI_IDLE 0
#define CRI_RUNNING 1

static char* idle_anim[] = {"Nolty.Idle","Anomaly.Idle"};
static char* running_anim[] = {"Nolty.Running","Anomaly.Running"};

typedef struct {
	uint8_t type;
	uint8_t aitype;
	
	float x;
	float y;

	float velocity;

	float anim_time;

	int state;

	float move_x;
	float move_y;

	float hp;
} HumanCore;

typedef struct {
	CRITTER_BASE;
	HumanCore c;
	
	float face_x;
	float face_y;

	int anim;

	AiFunc ai;	
} Human;

//-----------------------------------------------------------------------------
static void tick(Critter * c)
{	
	Human *cri = (Human *) c;	
	
	cri->ai(c);

	if (cri->c.hp <= 0) {
		return;
	}

	cri->c.x += cri->face_x * cri->c.velocity;
	cri->c.y += cri->face_y * cri->c.velocity;
	cri->c.anim_time += 1.0 / 30.0f;

	if (cri->c.state == CRI_RUNNING) {
		cri->c.velocity = 0.08;
		cri->face_x = cri->c.move_x - cri->c.x;
		cri->face_y = cri->c.move_y - cri->c.y;
		float len = sqrt(cri->face_x * cri->face_x + cri->face_y * cri->face_y);
		if (len < 0.08) {
			cri->c.state = CRI_IDLE;
		}
		cri->face_x /= len;
		cri->face_y /= len;
	}	
	if (cri->c.state == CRI_IDLE) {
		cri->c.velocity = 0;
	}	
}


//-----------------------------------------------------------------------------
static void draw(Critter * c, float time_delta)
{
	Human *cri = (Human *) c;
	IsoAnim *anim = NULL;
	if (cri->c.hp <= 0) {
		Sprite *s = blit_get_sprite("Blurred_001");
		float wx, wy;
		iso_world2screen(cri->c.x, cri->c.y, &wx, &wy);
		blit_sprite(s, wx - 50, wy - 50);		
		return;
	}
	if (cri->c.state == CRI_IDLE) {
		anim = isoanim_get(idle_anim[cri->anim]);
	} else if (cri->c.state == CRI_RUNNING) {
		anim = isoanim_get(running_anim[cri->anim]);
	}

	isoanim_blit_frame(anim, cri->c.x, cri->c.y, cri->c.anim_time, cri->face_x, cri->face_y);
}


//-----------------------------------------------------------------------------
static void damage(Critter * c, float amount)
{
	Human *cri = (Human *) c;
	cri->c.hp -= amount;	
}

//-----------------------------------------------------------------------------
static void order(Critter * c, Netcmd * command)
{
	Human *cri = (Human *) c;
	switch (command->header.type) {
	case NETCMD_MOVECRITTER:{
			Netcmd_MoveCritter *move = (Netcmd_MoveCritter *) command;
			cri->c.state = CRI_RUNNING;
			cri->c.move_x = move->move_x;
			cri->c.move_y = move->move_y;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// rebuilds whole struct from struct's core
static void human_rebuild(Critter *c)
{			
	Human *cri = (Human *)c;
	cri->face_x = cri->c.move_x - cri->c.x;
	cri->face_y = cri->c.move_y - cri->c.y;
	float len = sqrt(cri->face_x * cri->face_x + cri->face_y * cri->face_y);		
	cri->face_x /= len;
	cri->face_y /= len;
	
	cri->vtable->set_ai((Critter*)cri, cri->c.aitype);	
}

GENERIC_CORE_PACK_SIZE(human_pack_size, Human)

GENERIC_CORE_SERIALIZER(Critter, Human)
GENERIC_CORE_DESERIALIZER(Critter, Human, human_rebuild)

//-----------------------------------------------------------------------------
static void get_viewpoint(Critter * c, float *x, float *y)
{
	Human *cri = (Human *) c;
	*x = cri->c.x;
	*y = cri->c.y;
}

//-----------------------------------------------------------------------------
static float get_hp(Critter * c)
{
	Human *cri = (Human *) c;
	return cri->c.hp;
}

//-----------------------------------------------------------------------------
static float get_velocity(Critter * c)
{
	Human *cri = (Human *) c;
	return cri->c.velocity;
}

//-----------------------------------------------------------------------------
static void set_ai(Critter * c, int ai)
{
	Human *cri = (Human *) c;
	cri->ai = ai_funcs[ai];
	cri->c.aitype = ai;
}

//-----------------------------------------------------------------------------
static CritterVTable vtable;
void human_init_vtable()
{	
	vtable.tick = tick;
	vtable.order = order;
	vtable.draw = draw;
	vtable.damage = damage;
	vtable.serialize = serialize;
	vtable.deserialize = deserialize;
	vtable.get_viewpoint = get_viewpoint;
	vtable.get_hp = get_hp;
	vtable.get_velocity = get_velocity;
	vtable.set_ai = set_ai;
}

//-----------------------------------------------------------------------------
Critter* new_human(int gfx)
{
	Human *h = (Human *) malloc(sizeof(Human));
	h->vtable = &vtable;
	h->c.type = CRITTER_HUMAN;
	return (Critter *)h;
}

//-----------------------------------------------------------------------------
Critter *create_human(int gfx, float x, float y,int anim)
{
	Human *h = (Human *) new_human(gfx);
	h->c.x = x;
	h->c.y = y;
	h->c.velocity = 0;
	h->c.state = CRI_IDLE;
	h->c.anim_time = 0;
	h->c.hp = 100;
	
	h->face_x = 0;
	h->face_y = 1;
	h->anim = anim;
	h->ai = ai_noop;
	return (Critter *)h;
}


