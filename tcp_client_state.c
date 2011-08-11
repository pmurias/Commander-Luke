#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glfw.h>

#include "network.h"
#include "queue.h"
#include "socket.h"

typedef struct  {
	Queue *out;	
	float last_post_time;
	float post_delay;	
	float wait_start;
	
	Queue *in;
	char *read_buf;
	int num_read_bytes;
	uint32_t msg_size;
	int waiting;
		
	TcpClient *socket;
	char client_id;
} TcpClientState;

//-----------------------------------------------------------------------------
static void tick(void *d)
{
	TcpClientState *state = (TcpClientState *) d;
	tcpclient_select(state->socket);
}

//-----------------------------------------------------------------------------
static void logic_tick(void *d)
{
	TcpClientState *state = (TcpClientState *) d;

	float currTime = glfwGetTime();
	if (state->waiting || currTime < state->last_post_time + state->post_delay)
		return;
	state->last_post_time = currTime;

	static char packet[8196];
	uint32_t packet_size = 4;	
	Netcmd *cmd;	
		
	while ((cmd = queue_first(state->out)) != NULL) {		
		memcpy(packet + packet_size, cmd, command_size(cmd));
		packet_size += command_size(cmd);					
		queue_pop(state->out);
	}
		
	uint32_t data_size = packet_size - sizeof(uint32_t);
	memcpy(packet, &data_size, sizeof(uint32_t));	
		
	tcpclient_write(state->socket, packet, packet_size);
	state->waiting = 1;	
        state->wait_start = glfwGetTime();
}

//-----------------------------------------------------------------------------
static void add_command(void *d, Netcmd *cmd)
{	
	TcpClientState *state = (TcpClientState *) d;
	
	Netcmd *cmdcopy = malloc(command_size(cmd));
	memcpy(cmdcopy, cmd, command_size(cmd));	

	queue_push(state->out, cmdcopy);	
}

//-----------------------------------------------------------------------------
static void client_read(TcpClient * socket, char *buf, int len)
{
	TcpClientState *state = tcpclient_get_user_data(socket);	
	
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
				int cmdsize = command_size((Netcmd*)(state->read_buf+off));												
				Netcmd *cmd = malloc(cmdsize);
				memcpy(cmd, state->read_buf+off, cmdsize);
				off += cmdsize;
				
				queue_push(state->in, cmd);				
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

//-----------------------------------------------------------------------------
static void client_disconnect(TcpClient * socket)
{
	printf("Disconnect\n");
}

//-----------------------------------------------------------------------------
static Netcmd *get_command(void *d)
{
	TcpClientState *state = d;
	if (!state->waiting && queue_first(state->in)) {
		Netcmd *cmd = queue_first(state->in);
						
		Netcmd *cmdcopy = malloc(command_size(cmd));
		memcpy(cmdcopy, cmd, command_size(cmd));
		
		queue_pop(state->in);
		return cmdcopy;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
static void cleanup(void *d)
{
	TcpClientState *state = (TcpClientState *) d;
	
	queue_clear(state->in);
	queue_clear(state->out);	
}

//-----------------------------------------------------------------------------
static uint8_t get_id(void *d)
{
	TcpClientState *state = (TcpClientState *) d;
	return state->client_id;
}

//-----------------------------------------------------------------------------
NetworkType *new_tcp_client_state(char *ip)
{
	NetworkType *tcp = malloc(sizeof(NetworkType));
	tcp->tick = tick;
	tcp->logic_tick = logic_tick;
	tcp->add_command = add_command;
	tcp->get_command = get_command;
	tcp->get_id = get_id;
	tcp->cleanup = cleanup;

	TcpClientState *state = (TcpClientState *) malloc(sizeof(TcpClientState));
	tcp->state = (void *)state;

	state->msg_size = 0;
	state->in = new_queue(1);	
	state->out = new_queue(1);
	state->last_post_time = 0;	
	state->post_delay = 0.05;
	state->waiting = 0;
	state->client_id = -1;

	state->socket = new_tcpclient();
	tcpclient_init(state->socket, 1234, ip);
	tcpclient_set_user_data(state->socket, tcp->state);

	printf("Connecting...\n");
	tcpclient_set_handlers(state->socket, &client_read, &client_disconnect);
	tcpclient_connect(state->socket);
	
	return tcp;
}

