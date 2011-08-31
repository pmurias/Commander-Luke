#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "critter.h"
#include "critters/human.h"

IntMap *critters;

//------------------------------------------------------------------------------
void critters_serialize(void **buf, uint32_t *size)
{
	*buf = malloc(sizeof(uint32_t));
	*size = sizeof(uint32_t);	
	uint32_t cri_count = critters->size - critters->free;
	memcpy(*buf, &cri_count, sizeof(uint32_t));
	printf("Sending %d critters\n", cri_count);
	
	void *cbuf;
	uint32_t csize;
	for (int i=0; i<critters->size; i++) {
		if (critters->keys[i]) {
			Critter *c = critters->data[i];
			c->vtable->serialize(c, &cbuf, &csize);
			*buf = realloc(*buf, *size + sizeof(uint32_t) + csize);
			memcpy(*buf + *size, &critters->keys[i], sizeof(uint32_t));
			memcpy(*buf + *size + sizeof(uint32_t), cbuf, csize);
			*size +=  sizeof(uint32_t) + csize;			
			free(cbuf);
		}
	}	
}

//------------------------------------------------------------------------------
void critters_deserialize(void *buf)
{
	uint32_t count;
	memcpy(&count, buf, sizeof(uint32_t));
		
	uint32_t off = sizeof(uint32_t);
	for (int i=0; i<count; i++) {
		uint32_t key;		
		memcpy(&key, buf+off, sizeof(uint32_t));
		off += sizeof(uint32_t);
		
		Critter *c = intmap_find(critters, key);
		uint8_t type = *(uint8_t*)(buf+off);
		switch (type) {
		case CRITTER_HUMAN:
			if (c == NULL) {
				c = new_human(0,0,0); // bad!
				intmap_ins(critters, key, c);				
			}
			c->vtable->deserialize(c, buf+off, human_pack_size());
			off += human_pack_size();			
			break;
		default:
			printf("error: unknown critter type!\n");
			exit(1);
		}
	}
}
