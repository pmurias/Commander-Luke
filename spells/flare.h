#ifndef __FLARE_H__
#define __FLARE_H__

#include "spell.h"

Spell *new_flare(int gfx);
Spell* create_flare(int gfx, float x, float y, float mx, float my);
uint32_t flare_pack_size(void);
void flare_init_vtable(void);

#endif // __FLARE_H__