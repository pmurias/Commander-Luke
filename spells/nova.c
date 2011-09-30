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
	float fade;
} NovaCore;

typedef struct {
	SPELL_BASE;
	NovaCore c;
} Nova;

//-----------------------------------------------------------------------------
static void tick(Spell ** s)
{
	Nova *spell = (Nova *) * s;
        spell->c.fade -= 0.01;
	if (spell->c.fade < 0.01) {
		(*s)->vtable->free(s);
	}
}

//-----------------------------------------------------------------------------
static void draw(Spell * s, float time_delta)
{
	Nova *spell = (Nova *) s;

	float scale = 0.3;
	float r = 4.0*(1-spell->c.fade)+0.4;
        int n = 64;
	for (int i = 0; i < n; i++) {
		float a = (i * 2 * M_PI / n);

		float x = cos(a) * r + spell->c.x;
		float y = sin(a) * r + spell->c.y;

		float wx, wy;
		iso_world2screen(x, y, &wx, &wy);

		Sprite *nova = blit_get_sprite("./data/flare.png");
		nova->r = nova->g = nova->b = 1.0;
        isozbatch_add_sprite_scaled(nova, x, y, scale);
	//	blit_sprite_scaled(nova, wx, wy, scale);
	}

}

//-----------------------------------------------------------------------------
// rebuilds whole struct from struct's core
static void nova_rebuild(Spell * s)
{
	/* noop */
}

GENERIC_CORE_PACK_SIZE(nova_pack_size, Nova)

GENERIC_CORE_SERIALIZER(Spell, Nova)
GENERIC_CORE_DESERIALIZER(Spell, Nova, nova_rebuild)
//-----------------------------------------------------------------------------
static void _free(Spell ** s)
{
	//Nova *spell = (Nova *) * s;
	free(*s);
	*s = NULL;
}

//-----------------------------------------------------------------------------
static SpellVTable vtable;
void nova_init_vtable()
{
	vtable.tick = tick;
	vtable.draw = draw;
	vtable.serialize = serialize;
	vtable.deserialize = deserialize;
	vtable.free = _free;
}

//-----------------------------------------------------------------------------
Spell *new_nova()
{
	Nova *spell = (Nova *) malloc(sizeof(Nova));
	spell->vtable = &vtable;
	spell->c.type = SPELL_FLARE;
	return (Spell *) spell;
}

//-----------------------------------------------------------------------------
Spell *create_nova(float x, float y)
{
	Nova *spell = (Nova *) new_nova();
	spell->c.x = x;
	spell->c.y = y;
	spell->c.fade = 1.0;
	return (Spell *) spell;
}
