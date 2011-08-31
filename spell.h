#ifndef __SPELL_H__
#define __SPELL_H__

#include "hashmap.h"

#define SPELL_BASE \
	SpellVTable* vtable;

struct Spell;
typedef struct Spell Spell;

typedef struct {
  void (*draw)(Spell *s,float time_delta);
  void (*tick)(Spell **s);
  void (*serialize)(Spell *spell, void** buf, uint32_t *size);
  void (*deserialize)(Spell *spell, void* buf, uint32_t size);
  void (*free)(Spell **s);  
} SpellVTable;

struct Spell {
    SPELL_BASE
};

#define SPELL_FLARE 1

extern uint32_t spell_uid;
extern IntMap *spells;

void spells_serialize(void **buf, uint32_t *size);
int spells_deserialize(void *buf);

#endif // __SPELL_H__