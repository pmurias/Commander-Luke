#ifndef __NOVA_H__
#define __NOVA_H__

#include "spell.h"

Spell *new_nova();
Spell* create_nova(float x, float y, int alien);
uint32_t nova_pack_size(void);
void nova_init_vtable(void);

#endif // __FLARE_H__
