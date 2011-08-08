#include "commands.h"

static int command_sizes[MAX_NETCMD];

void command_startup()
{	
	command_sizes[NETCMD_SETTILE] = sizeof(Netcmd_SetTile);	
}

int command_size(Netcmd *cmd) 
{
	return command_sizes[cmd->header.type];
}