#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

typedef struct {
	uint8_t type;	
} NetcmdHeader;

typedef struct {
	NetcmdHeader header;
	char data[1];
} Netcmd;

typedef struct {
	NetcmdHeader header;	
	int tile_x;
	int tile_y;
	int type;
} Netcmd_SetTile;

#pragma pack(pop)

#define NETCMD_SETTILE 1
#define MAX_NETCMD 2

void command_startup();
int command_size(Netcmd *);

#endif
