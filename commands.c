#include "commands.h"

void commands_startup(void)
{	
	netcmd_sizes[NETCMD_SETTILE] = sizeof(Netcmd_SetTile);
	netcmd_sizes[NETCMD_MOVECRITTER] = sizeof(Netcmd_MoveCritter);
	netcmd_sizes[NETCMD_SPAWNFLARE] = sizeof(Netcmd_SpawnFlare);
}
