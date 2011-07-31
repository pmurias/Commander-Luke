#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "str.h"

typedef struct
{
	uint16_t elem_size;
	uint32_t size;
	uint32_t free;
	Str *keys;
	char *data;
} HashMap;

extern uint32_t hashmap_h2(uint32_t k);
extern void hashmap_init(HashMap *hm, uint16_t elem_size, uint32_t size);
extern uint32_t hashmap_ins(HashMap *hm, Str *key, void *elem);
extern void hashmap_set(HashMap *hm, Str *key, void *elem);
extern void *hashmap_find(HashMap *hm, Str *key);
extern void hashmap_rem(HashMap *hm, Str *key);
extern void hashmap_resize(HashMap *hm, uint32_t new_siz);
extern void hashmap_print(HashMap *hm);


#endif // __HASHMAP_H__
