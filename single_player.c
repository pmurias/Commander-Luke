#include <stdlib.h>
#include <string.h>

#include "network.h"

typedef struct Command {
	struct Command *next;	
	Netcmd *cmd;
} Command;

typedef struct {
	Command *last;
	Command *first;
} Data;

static void noop(void *data)
{
}

static void add_command(void *data, Netcmd *cmd)
{
	Data *commands = (Data *) data;

	Command *c = (Command *) malloc(sizeof(Command));	
	c->cmd = malloc(command_size(cmd));
	memcpy(c->cmd, cmd, command_size(cmd));
	c->next = NULL;

	if (!commands->first) {
		commands->first = c;
		commands->last = c;
	} else {
		commands->last->next = c;
	}
}

static Netcmd *get_command(void *data)
{
	Data *commands = (Data *) data;
	if (!commands->first) {
		return NULL;
	} else {
		Command *ret = commands->first;
		commands->first = ret->next;
		if (commands->last == ret) {
			commands->last = NULL;
		}		
		return ret->cmd;
	}
}

static void cleanup(void *data)
{
	Data *commands = (Data *) data;

	Command *c = commands->first;
	while (c) {
		free(c->cmd);
		c = c->next;
	}

}

NetworkType *single_player_network(void)
{
	NetworkType *single = malloc(sizeof(NetworkType));
	single->tick = noop;
	single->logic_tick = noop;
	single->add_command = add_command;
	single->get_command = get_command;
	single->cleanup = cleanup;

	Data *data = (Data *) malloc(sizeof(Data));
	single->state = (void *)data;
	data->last = NULL;
	data->first = NULL;
	return single;
}
