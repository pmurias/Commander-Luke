#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "queue.h"

uint32_t *ticks_ptr;

static void noop(void *data)
{
	*ticks_ptr = 100;
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

static uint8_t get_id(void *data)
{
	return 0;
}

NetworkType *single_player_network(uint32_t *tickptr)
{
	NetworkType *single = malloc(sizeof(NetworkType));
	single->tick = noop;
	single->logic_tick = noop;
	single->add_command = add_command;
	single->get_command = get_command;
	single->get_id = get_id;
	single->cleanup = cleanup;
	ticks_ptr = tickptr;

	Queue *state = new_queue(0);
	single->state = (void *)state;	
	return single;
}
