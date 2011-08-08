#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

#define HASHMAP_MIN_SIZE 64

//-----------------------------------------------------------------------------
uint32_t hashmap_h2(uint32_t k)
{
	/* oh noes!!!!!!!111111oneoneoneeleven */
	return k;
}

//-----------------------------------------------------------------------------
HashMap *new_hashmap(uint16_t elem_size)
{
	HashMap *hm = malloc(sizeof(HashMap));
	hashmap_init(hm, elem_size, HASHMAP_MIN_SIZE);
	return hm;
}

//-----------------------------------------------------------------------------
void hashmap_init(HashMap *hm, uint16_t elem_size, uint32_t size)
{
	uint32_t i;
	hm->size = size;
	hm->data = malloc(size * elem_size);
	hm->keys = malloc(size * sizeof(Str));
	hm->free = size;
	hm->elem_size = elem_size;

	for (i=0; i<size; ++i)
		str_init(&hm->keys[i]);
}

//-----------------------------------------------------------------------------
static inline void _hashmap_write_elem(HashMap *hm, uint32_t h, Str *key, void *elem)
{
	memcpy(hm->data + h*hm->elem_size, elem, hm->elem_size);
	str_cpy(&hm->keys[h], key);
	hm->free--;
	if (hm->free <= hm->size>>2)
		hashmap_resize(hm, hm->size << 2);
}

//-----------------------------------------------------------------------------
uint32_t hashmap_ins(HashMap *hm, Str *key, void *elem)
{
	uint32_t h, k, i = key->hash % hm->size;
	for (k=0; k<hm->size; ++k) {
		h = hashmap_h2(i+k) % hm->size;
		if (hm->keys[h].len == 0) {
			_hashmap_write_elem(hm, h, key, elem);
			return 1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
void hashmap_set(HashMap *hm, Str *key, void *elem)
{
	uint32_t h, k, i = key->hash % hm->size;
	for (k=0; k<hm->size; ++k) {
		h = hashmap_h2(i+k) % hm->size;
		if (str_cmp(&hm->keys[h], key)) {
			memcpy(hm->data + h*hm->elem_size, elem, hm->elem_size);
			return;
		} else if (hm->keys[h].len == 0) {
			_hashmap_write_elem(hm, h, key, elem);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
void *hashmap_find(HashMap *hm, Str *key)
{
	uint32_t h, k, i = key->hash % hm->size;
	for (k=0; k<hm->size; ++k) {
		h = hashmap_h2(i+k) % hm->size;
		if (str_cmp(&hm->keys[h], key)) {
			return (hm->data + h*hm->elem_size);
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
void hashmap_rem(HashMap *hm, Str *key)
{
	uint32_t h, k, i = key->hash % hm->size;
	for (k=0; k<hm->size; ++k) {
		h = hashmap_h2(i+k) % hm->size;
		if (str_cmp(&hm->keys[h], key)) {
			hm->keys[h].len = 0;
			hm->free++;
			if (hm->free > 3 * (hm->size >> 2) && hm->size > HASHMAP_MIN_SIZE) {
				hashmap_resize(hm, hm->size >> 1);
			}
			return;
		}
	}
}


//-----------------------------------------------------------------------------
static uint32_t _hashmap_reloc(HashMap *hm, Str *key, void *elem)
{
	uint32_t h, k, i = key->hash % hm->size;
	for (k=0; k<hm->size; ++k) {
		h = hashmap_h2(i+k) % hm->size;
		if (hm->keys[h].len == 0) {
			memcpy(hm->data + h*hm->elem_size, elem, hm->elem_size);
			str_cpy(&hm->keys[h], key);
			return 1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
void hashmap_resize(HashMap *hm, uint32_t new_size)
{
	uint32_t i, oldSize = hm->size;
	char *oldData = hm->data;
	Str *oldKeys = hm->keys;

	hm->size = new_size;
	hm->data = malloc(hm->size*hm->elem_size);
	hm->keys = malloc(hm->size*sizeof(Str));
	hm->free += (new_size - oldSize);

	for (i=0; i<hm->size; ++i)
		str_init(&hm->keys[i]);

	for (i=0; i<oldSize; ++i) {
		if (oldKeys[i].len != 0) {
			_hashmap_reloc(hm, &oldKeys[i], oldData + i*hm->elem_size);
		}
		str_free(&oldKeys[i]);
	}

	free(oldData);
	free(oldKeys);
}
