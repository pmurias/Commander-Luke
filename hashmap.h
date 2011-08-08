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

HashMap *new_hashmap(uint16_t elem_size);
uint32_t hashmap_h2(uint32_t k);
void hashmap_init(HashMap *hm, uint16_t elem_size, uint32_t size);
uint32_t hashmap_ins(HashMap *hm, Str *key, void *elem);
void hashmap_set(HashMap *hm, Str *key, void *elem);
void *hashmap_find(HashMap *hm, Str *key);
void hashmap_rem(HashMap *hm, Str *key);
void hashmap_resize(HashMap *hm, uint32_t new_siz);
void hashmap_print(HashMap *hm);


#endif // __HASHMAP_H__
