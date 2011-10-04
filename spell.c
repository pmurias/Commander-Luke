#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "spell.h"
#include "spells/flare.h"
#include "spells/teleport.h"
#include "spells/nova.h"

uint32_t spell_uid = 1;
IntMap *spells;

//------------------------------------------------------------------------------
static void spell_serialzier(void *cptr, void **obuf, uint32_t *osize)
{
	Spell *c = (Spell*) cptr;
	c->vtable->serialize(c, obuf, osize);
}

//------------------------------------------------------------------------------
void spells_serialize(void **buf, uint32_t *size)
{	
	intmap_serialize(spells, spell_serialzier, buf, size);
}

//------------------------------------------------------------------------------
static uint32_t spell_deserializer(void *cptr, void *inbuf)
{
	Spell *c = (Spell*)cptr;
	uint8_t type = *(uint8_t*)(inbuf);
	switch (type) {
	case SPELL_FLARE:		
		c->vtable->deserialize(c, inbuf, flare_pack_size());
		return flare_pack_size();
		break;
	case SPELL_NOVA:		
		c->vtable->deserialize(c, inbuf, nova_pack_size());
		return flare_pack_size();
		break;
    case SPELL_TELE:		
		c->vtable->deserialize(c, inbuf, teleport_pack_size());
		return flare_pack_size();
		break;
	default:
		printf("error: unknown spell type!\n");
		exit(1);
	}
}

//------------------------------------------------------------------------------
static void *spell_ctor(void *inbuf)
{	
	uint8_t type = *(uint8_t*)(inbuf);
	switch (type) {
	case SPELL_FLARE:
		return new_flare(1);
    case SPELL_NOVA:
        return new_nova();
    case SPELL_TELE:
        return new_teleport(1);
	default:
		printf("error: unknown critter type!\n");
		exit(1);
	}
}

//------------------------------------------------------------------------------
int spells_deserialize(void *buf)
{
	return intmap_deserialize(spells, spell_deserializer, spell_ctor, buf);
}
