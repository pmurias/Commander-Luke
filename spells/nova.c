#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "spell.h"
#include "iso.h"
#include "serializers.h"
#include "critter.h"
#include <stdio.h>

#define MAXIMAL_HITS 64
typedef struct {
	uint8_t type;
	float x;
	float y;
	float fade;
	int team;
        uint32_t hit[MAXIMAL_HITS];
        int hitTop;
} NovaCore;

typedef struct {
	SPELL_BASE;
	NovaCore c;
} Nova;


static float radious(Nova * spell)
{
	return 4.0 * (1 - spell->c.fade) + 0.04;
}

//-----------------------------------------------------------------------------
static void tick(Spell ** s)
{
	Nova *spell = (Nova *) * s;
	spell->c.fade -= 0.01;
	if (spell->c.fade < 0.01) {
		(*s)->vtable->free(s);
	}

	for (int i = 0; i < critters->size; i++) {
		if (critters->keys[i]) {
			Critter *c = critters->data[i];
			float x, y;

			c->vtable->get_viewpoint(c, &x, &y);

			float fx = x - spell->c.x;
			float fy = y - spell->c.y;
			float len = sqrt(fx * fx + fy * fy);
			if (fabs(len - radious(spell)) < 0.1 && critters->keys[i] != spell->c.team) {
                                int j;
                                for (j=0;j<=spell->c.hitTop;j++) {
                                    if (critters->keys[i] == spell->c.hit[j]) {
                                      break;
                                    }
                                }

                                if (j == spell->c.hitTop+1 && spell->c.hitTop < MAXIMAL_HITS-1) {
                                        spell->c.hit[++spell->c.hitTop] = critters->keys[i];
					c->vtable->damage(c, 9001);

                                }
			}
		}
	}
}

//-----------------------------------------------------------------------------
static void draw(Spell * s, float time_delta)
{
	Nova *spell = (Nova *) s;

	float scale = 0.3;
	float r = radious(spell);
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
		//      blit_sprite_scaled(nova, wx, wy, scale);
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
Spell *create_nova(float x, float y, int team)
{
	Nova *spell = (Nova *) new_nova();
	spell->c.x = x;
	spell->c.y = y;
	spell->c.fade = 1.0;
	spell->c.team = team;
        spell->c.hitTop = -1;
	return (Spell *) spell;
}
