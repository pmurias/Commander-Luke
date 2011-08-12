#include <math.h>
#include <stdlib.h>

#include "spell.h"
#include "iso.h"

typedef struct {
	SPELL_BASE;	
	float x;
	float y;

	float velocity;
	
	float move_x;
	float move_y;		
	
	float scale;
	float fade;
	IsoLight *light;
} Flare;


//-----------------------------------------------------------------------------
static void tick(Spell **s)
{
	Flare *spell = (Flare *) *s;
	
	float fx = spell->move_x - spell->x;
	float fy = spell->move_y - spell->y;
	float len = sqrt(fx*fx + fy*fy);
	if (len < spell->velocity) {
		spell->scale *= 1.15;
		spell->scale *= 1.05;
		spell->fade *= 0.6;
		if (spell->fade < 0.05) {
			(*s)->vtable->free(s);
		}
	} else {	
		fx /= len;
		fy /= len;
		spell->x += fx * spell->velocity;
		spell->y += fy * spell->velocity;	
	}
}

//-----------------------------------------------------------------------------
static void draw(Spell *s, float time_delta)
{
	Flare *spell = (Flare *) s;
	
	float wx, wy;
	iso_world2screen(spell->x, spell->y, &wx, &wy);
		
	Sprite *flare = blit_get_sprite("./data/flare.png");
	flare->r = flare->g = flare->b = spell->fade;
	blit_sprite_scaled(flare, wx, wy, spell->scale);
	
	if (spell->light) {
		spell->light->x = spell->x;
		spell->light->y = spell->y;
	}
}

//-----------------------------------------------------------------------------
static void _free(Spell **s)
{
	Flare *spell = (Flare *) *s;
	free_isolight(&spell->light);
	free(*s);
	*s = NULL;
}

//-----------------------------------------------------------------------------
static SpellVTable vtable;
void flare_init_vtable()
{	
	vtable.tick = tick;	
	vtable.draw = draw;
	vtable.free = _free;
}

//-----------------------------------------------------------------------------
Spell *new_flare(float x, float y, float mx, float my)
{
	Flare *spell = (Flare *) malloc(sizeof(Flare));
	spell->vtable = &vtable;
	spell->x = x;
	spell->y = y;
	spell->velocity = 0.3;
	spell->move_x = mx;
	spell->move_y = my;
	spell->scale = 0.2;
	spell->fade = 200;
	spell->light = new_isolight();
	spell->light->x = x;
	spell->light->y = y;
	spell->light->range = 2;
	spell->light->r = 0.5;
	spell->light->g = 0.7;
	spell->light->b = 1.0;
	return (Spell *)spell;
}