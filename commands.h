#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdint.h>
#include "network.h"

#pragma pack(push)
#pragma pack(1)

typedef struct {
	NetcmdHeader header;	
	int tile_x;
	int tile_y;
	int type;
} Netcmd_SetTile;

#pragma pack(pop)

#define NETCMD_SETTILE 1

void commands_startup(void);

#endif
