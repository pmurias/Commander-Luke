#include "commands.h"

void commands_startup(void)
{	
	netcmd_sizes[NETCMD_SETTILE] = sizeof(Netcmd_SetTile);
	netcmd_sizes[NETCMD_MOVECRITTER] = sizeof(Netcmd_MoveCritter);
}
