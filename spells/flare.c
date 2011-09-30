#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "spell.h"
#include "iso.h"
#include "serializers.h"
#include "critter.h"
#include <stdio.h>

typedef struct {
	uint8_t type;
	float x;
	float y;

	float velocity;

	float move_x;
	float move_y;

	float dist;
	float z;
	
	float fade;	
	int exploded;
} FlareCore;

typedef struct {
	SPELL_BASE;
	FlareCore c;
	
	float angle;
	float scale;
	IsoLight *light;
} Flare;

//-----------------------------------------------------------------------------
static void tick(Spell ** s)
{
	Flare *spell = (Flare *) * s;

	float fx = spell->c.move_x - spell->c.x;
	float fy = spell->c.move_y - spell->c.y;
	float len = sqrt(fx * fx + fy * fy);
	if (spell->c.dist < 0) {
		spell->c.dist = len;
	}

	if (len < spell->c.velocity) {
		if (!spell->c.exploded) {
			spell->c.exploded = 1;
			for (int i = 0; i < critters->size; i++) {
				if (critters->keys[i]) {
					Critter *c = critters->data[i];
					float x, y;
	
					c->vtable->get_viewpoint(c, &x, &y);
	
					float fx = x - spell->c.x;
					float fy = y - spell->c.y;
					float len = sqrt(fx * fx + fy * fy);
					if (len < 0.75)
						c->vtable->damage(c, 10);
				}
			}
		}
		spell->c.z = 0;		
		spell->c.fade *= 0.6;
		
		spell->angle += 10;		
		spell->scale *= 1.10;		
		spell->light->range *= 1.05;
		if (spell->c.fade < 0.05) {
			(*s)->vtable->free(s);
		}
	} else {
		fx /= len;
		fy /= len;
		spell->c.x += fx * spell->c.velocity;
		spell->c.y += fy * spell->c.velocity;
		spell->c.z = sin(M_PI * (0.3 + 0.7 * (spell->c.dist - len) / spell->c.dist)) * spell->c.dist * 20;
	}
}

//-----------------------------------------------------------------------------
static void draw(Spell * s, float time_delta)
{
	Flare *spell = (Flare *) s;

	float wx, wy;
	iso_world2screen(spell->c.x, spell->c.y, &wx, &wy);

	Sprite *flare = blit_get_sprite("./data/flare.png");
    isozbatch_add_sprite_ex(flare, spell->c.x, spell->c.y, spell->c.z, 0, 0, spell->scale, spell->angle, 0,
            spell->c.fade, spell->c.fade, spell->c.fade);

	if (spell->light) {
		spell->light->x = spell->c.x;
		spell->light->y = spell->c.y;
	}
}
	
//-----------------------------------------------------------------------------
// rebuilds whole struct from struct's core
static void flare_rebuild(Spell *s)
{			
	/* noop */				
}

GENERIC_CORE_PACK_SIZE(flare_pack_size, Flare)

GENERIC_CORE_SERIALIZER(Spell, Flare)
GENERIC_CORE_DESERIALIZER(Spell, Flare, flare_rebuild)

//-----------------------------------------------------------------------------
static void _free(Spell ** s)
{
	Flare *spell = (Flare *) * s;
	if (spell->light) {
		spell->light->range = 0;
		free_isolight(&spell->light);
	}
	free(*s);
	*s = NULL;
}

//-----------------------------------------------------------------------------
static SpellVTable vtable;
void flare_init_vtable()
{
	vtable.tick = tick;
	vtable.draw = draw;
	vtable.serialize = serialize;
	vtable.deserialize = deserialize;
	vtable.free = _free;
}

//-----------------------------------------------------------------------------
Spell *new_flare(int gfx)
{
	Flare *spell = (Flare *) malloc(sizeof(Flare));
	spell->vtable = &vtable;
	spell->c.type = SPELL_FLARE;
			
	if (gfx) {
		spell->scale = 0.2;	
		spell->angle = rand()%1000;
		spell->light = new_isolight();
		spell->light->x = spell->c.x;
		spell->light->y = spell->c.y;
		spell->light->range = 1;
		spell->light->r = 0.5;
		spell->light->g = 0.7;
		spell->light->b = 1.0;
	}
	
	return (Spell *) spell;
}

//-----------------------------------------------------------------------------
Spell* create_flare(int gfx, float x, float y, float mx, float my)
{
	Flare *spell = (Flare *)new_flare(gfx);
	spell->c.x = x;
	spell->c.y = y;
	spell->c.move_x = mx;
	spell->c.move_y = my;	
	spell->c.velocity = 0.3;	
	spell->c.z = 30;
	spell->c.dist = -1;	
	spell->c.fade = 200;
	spell->c.exploded = 0;	
	
	if (gfx) {
		spell->light->x = spell->c.x;
		spell->light->y = spell->c.y;
	}
	
	return (Spell*)spell;
}

