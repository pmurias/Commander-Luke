#ifndef __FLARE_H__
#define __FLARE_H__

#include "spell.h"

Spell *new_flare(float x, float y, float mx, float my);
void flare_init_vtable(void);

#endif // __FLARE_H__