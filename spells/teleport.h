#ifndef __TELEPORT_H__
#define __TELEPORT_H__

#include "spell.h"

Spell *new_teleport(int gfx);
Spell* create_teleport(int gfx, int caster, float x, float y, float mx, float my);
uint32_t teleport_pack_size(void);
void teleport_init_vtable(void);

#endif // __TELEPORT_H__
