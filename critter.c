#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "critter.h"
#include "critters/human.h"

IntMap *critters;

//------------------------------------------------------------------------------
static void crit_serialzier(void *cptr, void **obuf, uint32_t *osize)
{
	Critter *c = (Critter*) cptr;
	c->vtable->serialize(c, obuf, osize);
}

//------------------------------------------------------------------------------
void critters_serialize(void **buf, uint32_t *size)
{	
	intmap_serialize(critters, crit_serialzier, buf, size);
}

//------------------------------------------------------------------------------
static uint32_t crit_deserializer(void *cptr, void *inbuf)
{
	Critter *c = (Critter*)cptr;
	uint8_t type = *(uint8_t*)(inbuf);
	switch (type) {
	case CRITTER_HUMAN:		
		c->vtable->deserialize(c, inbuf, human_pack_size());
		return human_pack_size();
		break;
	default:
		printf("error: unknown critter type!\n");
		exit(1);
	}
}

//------------------------------------------------------------------------------
static void *crit_ctor(void *inbuf)
{	
	uint8_t type = *(uint8_t*)(inbuf);
	switch (type) {
	case CRITTER_HUMAN:				
		return new_human(0,0,0); // bad! 		
	default:
		printf("error: unknown critter type!\n");
		exit(1);
	}
}

//------------------------------------------------------------------------------
void critters_deserialize(void *buf)
{
	intmap_deserialize(critters, crit_deserializer, crit_ctor, buf);
}
