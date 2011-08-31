#ifndef __CRITTER_H__
#define __CRITTER_H__


#include "commands.h"
#include "hashmap.h"

#define CRITTER_BASE \
	CritterVTable* vtable;

struct CritterVTable; 
typedef struct CritterVTable CritterVTable;

typedef struct {
    CRITTER_BASE
} Critter ;

struct CritterVTable {
  void (*draw)(Critter* critter,float time_delta); // The critter displays itself
  void (*tick)(Critter* critter); // The critter update it's state: walks, shoots etc.
  void (*order)(Critter* critter,Netcmd* command); // Orders the critter to execute command
  void (*serialize)(Critter *critter, void** buf, uint32_t *size);
  void (*deserialize)(Critter *critter, void* buf, uint32_t size);  
  void (*get_viewpoint)(Critter* critter,float *x,float *y);
  void (*damage)(Critter*,float hp);
  float (*get_hp)(Critter*);
  float (*get_velocity)(Critter*);
  void (*set_ai)(Critter*, int ai);
};

#define CRITTER_HUMAN 1

//HACK
extern IntMap *critters;

void critters_serialize(void **buf, uint32_t *size);
int critters_deserialize(void *buf);

#endif
