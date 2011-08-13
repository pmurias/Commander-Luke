#ifndef __CRITTER_H__
#define __CRITTER_H__


#include "commands.h"

#define CRITTER_BASE \
	CritterVTable* vtable;

struct CritterVTable; 
typedef struct CritterVTable CritterVTable;

typedef struct {
    CRITTER_BASE
} Critter ;

struct CritterVTable {
  void (*draw)(Critter* critter,float time_delta); // The critter displays itself
  void (*think)(Critter* critter); // The critter AI gives itself orders
  void (*tick)(Critter* critter); // The critter update it's state: walks, shoots etc.
  void (*order)(Critter* critter,Netcmd* command); // Orders the critter to execute command
  void (*get_viewpoint)(Critter* critter,float *x,float *y);
  void (*damage)(Critter*,float hp);
  float (*get_hp)(Critter*);
};

//HACK
#define MAX_CLIENTS 20
extern Critter *cri[MAX_CLIENTS];

#endif
