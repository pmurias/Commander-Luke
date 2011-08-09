#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "queue.h"

static void noop(void *data)
{
}

static void add_command(void *data, Netcmd *cmd)
{
	Netcmd *cmdcopy = malloc(command_size(cmd));
	memcpy(cmdcopy, cmd, command_size(cmd));
	queue_push((Queue*)data, cmdcopy);	
}

static Netcmd *get_command(void *data)
{
	return queue_pop(data);	
}

static void cleanup(void *data)
{
	queue_clear((Queue*)data);
}

NetworkType *single_player_network(void)
{
	NetworkType *single = malloc(sizeof(NetworkType));
	single->tick = noop;
	single->logic_tick = noop;
	single->add_command = add_command;
	single->get_command = get_command;
	single->cleanup = cleanup;

	Queue *state = new_queue(0);
	single->state = (void *)state;	
	return single;
}
