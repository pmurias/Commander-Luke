#ifndef __HUMAN_H__
#define __HUMAN_H__

#include "critter.h"

Critter* new_human(int gfx);
Critter *create_human(int gfx, float x,float y,int anim);
uint32_t human_pack_size(void);
void human_init_vtable(void);

#endif
