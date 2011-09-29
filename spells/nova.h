#ifndef __NOVA_H__
#define __NOVA_H__

#include "spell.h"

Spell *new_nova(int gfx);
Spell* create_nova(int gfx, float x, float y, float mx, float my);
uint32_t nova_pack_size(void);
void nova_init_vtable(void);

#endif // __FLARE_H__
