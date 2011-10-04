#include "commands.h"

void commands_startup(void)
{	
	netcmd_sizes[NETCMD_SETTILE] = sizeof(Netcmd_SetTile);
	netcmd_sizes[NETCMD_MOVECRITTER] = sizeof(Netcmd_MoveCritter);
	netcmd_sizes[NETCMD_SPAWNFLARE] = sizeof(Netcmd_SpawnFlare);
	netcmd_sizes[NETCMD_SPAWNNOVA] = sizeof(Netcmd_SpawnNova);
	netcmd_sizes[NETCMD_SPAWNTELEPORT] = sizeof(Netcmd_SpawnTeleport);
	netcmd_sizes[NETCMD_SETLOGIN] = sizeof(Netcmd_SetLogin);
}
