#include "str.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glfw.h>

#include "network.h"
#include "socket.h"
#include "commands.h"

typedef struct CmdNode {
	struct CmdNode *next;		
	Netcmd *netcmd;
} CmdNode;

typedef struct {
	CmdNode *last;
	CmdNode *first;
} CmdNodeQueue;

typedef struct  {
	CmdNodeQueue out;
	float last_post_time;
	float post_delay;	
        float wait_start;
	
	CmdNodeQueue in;	
	char *read_buf;
	int num_read_bytes;
	uint32_t msg_size;
	int waiting;
		
	TcpClient *client;
	char client_id;
} TcpNetworkState;

static void tick(void *d)
{
	TcpNetworkState *state = (TcpNetworkState *) d;
	tcpclient_select(state->client);
}

static void logic_tick(void *d)
{
	TcpNetworkState *state = (TcpNetworkState *) d;

	float currTime = glfwGetTime();
	if (state->waiting || currTime < state->last_post_time + state->post_delay)
		return;
	state->last_post_time = currTime;

	static char packet[8196];
	uint32_t packet_size = 4;	
	CmdNode *toRemove, *c;
		
	while (state->out.first) {				
		c = state->out.first;
		memcpy(packet + packet_size, c->netcmd, command_size(c->netcmd));
		packet_size += command_size(c->netcmd);		
					
		state->out.first = state->out.first->next;
		
		toRemove = c;		
		free(toRemove->netcmd);
		free(toRemove);
	}
		
	uint32_t data_size = packet_size - sizeof(uint32_t);
	memcpy(packet, &data_size, sizeof(uint32_t));	
		
	tcpclient_write(state->client, packet, packet_size);
	state->waiting = 1;	
        state->wait_start = glfwGetTime();
}

static void append_command(CmdNodeQueue *q, CmdNode *c)
{
	if (!q->first) {
		q->first = c;
		q->last = c;
	} else {
		q->last->next = c;
		q->last = c;
	}
}

static void add_command(void *d, Netcmd *cmd)
{	
	TcpNetworkState *state = (TcpNetworkState *) d;

	CmdNode *c = (CmdNode *) malloc(sizeof(CmdNode));
	c->next = NULL;
	c->netcmd = malloc(command_size(cmd));
	memcpy(c->netcmd, cmd, command_size(cmd));	

	append_command(&state->out, c);
}


void client_read(TcpClient * client, char *buf, int len)
{
	TcpNetworkState *state = tcpclient_get_user_data(client);	
	
	int i = 0;
	while (i != len) {
		/* greeting */
		if (state->client_id == -1) {
			state->client_id = *(buf+i);
			i += 1;			
			continue;
		}
		
		/* reading packet */
		if (state->msg_size == 0) {
			state->msg_size = *(uint32_t*)(buf + i);			
			state->read_buf = malloc(state->msg_size);
			state->num_read_bytes = sizeof(uint32_t);
			i += sizeof(uint32_t);						
		}
		
		int nbytes = state->msg_size - state->num_read_bytes;		
		int rbytes = (len - i) < nbytes ? (len - i) : nbytes;
		memcpy(state->read_buf + state->num_read_bytes, buf + i, rbytes);		
		i += rbytes;
		state->num_read_bytes += rbytes;		
			
		if (state->msg_size && state->num_read_bytes == state->msg_size) {				
			/* parse message to chain fo commands */			
			int off = 4;
			while (off != state->msg_size) {
				CmdNode *c = malloc(sizeof(CmdNode));
				int cmdsize = command_size((Netcmd*)(state->read_buf+off));								
				c->next = NULL;					
				c->netcmd = malloc(cmdsize);
				memcpy(c->netcmd, state->read_buf+off, cmdsize);
				off += cmdsize;
				
				append_command(&state->in, c);				
			}
								
			state->waiting = 0;

                        printf("Waited %f %d\n",glfwGetTime()-state->wait_start,state->msg_size);
			free(state->read_buf);
			state->msg_size = 0;
			if (i != len) {
				printf("Holy Shit! Only one set of commands should arrive from server! Go and find the bug. Now.\n");
				exit(1);
			}
		}
	}
		
	
}

void client_disconnect(TcpClient * client)
{
	printf("Disconnect\n");
}

static Netcmd *get_command(void *d)
{
	TcpNetworkState *state = d;
	if (!state->waiting && state->in.first) {
		CmdNode *ret = state->in.first;
		state->in.first = ret->next;
		
		Netcmd *cmd = malloc(command_size(ret->netcmd));
		memcpy(cmd, ret->netcmd, command_size(ret->netcmd));
		
		free(ret->netcmd);
		free(ret);
		return cmd;
	}
	return NULL;
}

static void cleanup(void *d)
{
	TcpNetworkState *state = (TcpNetworkState *) d;
	
	CmdNode *c = state->in.first;
	while (c) {
		free(c->netcmd);		
		c = c->next;
	}
	c = state->out.first;
	while (c) {
		free(c->netcmd);		
		c = c->next;
	}
}

NetworkType *tcp_network(char *ip)
{
	NetworkType *tcp = malloc(sizeof(NetworkType));
	tcp->tick = tick;
	tcp->logic_tick = logic_tick;
	tcp->add_command = add_command;
	tcp->get_command = get_command;
	tcp->cleanup = cleanup;

	TcpNetworkState *state = (TcpNetworkState *) malloc(sizeof(TcpNetworkState));
	tcp->state = (void *)state;

	state->msg_size = 0;
	state->in.last = NULL;
	state->in.first = NULL;
	state->out.last = NULL;
	state->out.first = NULL;
	state->last_post_time = 0;	
	state->post_delay = 0.05;
	state->waiting = 0;
	state->client_id = -1;

	state->client = new_tcpclient();
	tcpclient_init(state->client, 1234, ip);
	tcpclient_set_user_data(state->client, tcp->state);

	printf("Connecting...\n");
	tcpclient_set_handlers(state->client, &client_read, &client_disconnect);
	tcpclient_connect(state->client);
	
	return tcp;
}
