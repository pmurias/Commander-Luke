#ifndef __SPELL_H__
#define __SPELL_H__

#define SPELL_BASE \
	SpellVTable* vtable;

struct Spell;
typedef struct Spell Spell;

typedef struct {
  void (*draw)(Spell *s,float time_delta);
  void (*tick)(Spell **s);
  void (*free)(Spell **s);
} SpellVTable;

struct Spell {
    SPELL_BASE
};

#endif // __SPELL_H__