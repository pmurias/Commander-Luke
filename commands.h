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

typedef struct {
	NetcmdHeader header;	
	uint8_t sender;
	float move_x;
	float move_y;
} Netcmd_MoveCritter;

typedef struct {
	NetcmdHeader header;	
	uint8_t sender;
	float x;
	float y;
	float target_x;
	float target_y;
} Netcmd_SpawnFlare;

#pragma pack(pop)

#define NETCMD_SETTILE 1
#define NETCMD_MOVECRITTER 2
#define NETCMD_SPAWNFLARE 3

void commands_startup(void);

#endif
