#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "iso.h"
#include "critter.h"
#include "rand.h"

#define CRI_IDLE 0
#define CRI_RUNNING 1

typedef struct {
	CRITTER_BASE;
        float x;
	float y;
        float face_x;
        float face_y;
	int state;
        int i;
	float move_x;
	float move_y;
	float next_change;
        float velocity;
} Blurred;

static void tick(Critter * c)
{
	Blurred *cri = (Blurred *) c;
	cri->x += cri->face_x * cri->velocity;
	cri->y += cri->face_y * cri->velocity;

	if (cri->state == CRI_RUNNING) {
                if (cri->velocity <= 0.12) {
                  cri->velocity += 0.001;
                }
		cri->face_x = cri->move_x - cri->x;
		cri->face_y = cri->move_y - cri->y;
		float len = sqrt(cri->face_x * cri->face_x + cri->face_y * cri->face_y);
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

static void draw(Critter * c, float time_delta)
{
	Blurred *cri = (Blurred *) c;
	float wx, wy;
	iso_world2screen(cri->x, cri->y, &wx, &wy);

	while (time_delta >= cri->next_change) {
		time_delta -= cri->next_change;
                
		cri->next_change = (cri->velocity ? 0.15 : 0.3) + (rand_rand() % 3) * 0.05;
		cri->i = (cri->i + 1) % 2;
	}
	cri->next_change -= time_delta;

	Sprite *s;
	if (cri->i) {
		s = blit_get_sprite("Blurred_001");
	} else {
		s = blit_get_sprite("Blurred_002");
	}
	blit_sprite(s, wx-50, wy-50);

}


static void order(Critter * c, Netcmd * command)
{
	Blurred *cri = (Blurred *) c;
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

static void get_viewpoint(Critter * c, float *x, float *y)
{
	Blurred *cri = (Blurred *) c;
	*x = cri->x;
	*y = cri->y;
}

static CritterVTable *vtable;
void blurred_init_vtable()
{
	vtable = malloc(sizeof(CritterVTable));
	vtable->tick = tick;
	vtable->order = order;
	vtable->draw = draw;
	vtable->get_viewpoint = get_viewpoint;
}

Critter *new_blurred(float x, float y)
{
	Blurred *h = (Blurred *) malloc(sizeof(Blurred));
	h->vtable = vtable;
	h->x = x;
	h->y = y;
	h->move_x = x;
	h->move_y = y;
	h->i = 0;
        h->next_change = 0;
        h->velocity = 0;
	return (Critter *) h;
}
