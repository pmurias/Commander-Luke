#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glfw.h>

#include "network.h"
#include "queue.h"
#include "socket.h"
#include "tcp_network.h"

typedef struct  {
	Queue *out;	
	float last_post_time;
	float post_delay;	
	float wait_start;
	
	Queue *in;
	char *read_buf;
	int num_read_bytes;
	uint32_t msg_size;
	int msg_type;
	int waiting;
		
	TcpClient *socket;
	char client_id;
	int ready;
	
	ClientSnapshotCallback snapshot_callback;
} TcpClientState;

//-----------------------------------------------------------------------------
static void tick(void *d)
{
	TcpClientState *state = (TcpClientState *) d;
	tcpclient_select(state->socket);
}

static char packet[8196];

//-----------------------------------------------------------------------------
static void logic_tick(void *d)
{
	TcpClientState *state = (TcpClientState *) d;

	float currTime = glfwGetTime();
	if (state->waiting || currTime < state->last_post_time + state->post_delay)
		return;
	state->last_post_time = currTime;
	
	uint32_t packet_size = 5;	
	Netcmd *cmd;
		
	while ((cmd = queue_first(state->out)) != NULL) {
		memcpy(packet + packet_size, cmd, command_size(cmd));
		packet_size += command_size(cmd);	
		queue_pop(state->out);
	}
		
	uint32_t data_size = packet_size - sizeof(uint32_t)-1;
	packet[0] = TCP_MSG_CMDS;
	memcpy(packet+1, &data_size, sizeof(uint32_t));
		
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
static void send_confirmation(TcpClientState *state)
{
	packet[0] = TCP_MSG_CONFIRM;
	memset(packet+1, 0, 4);
	tcpclient_write(state->socket, packet, 5);
}

//-----------------------------------------------------------------------------
static void process_message(TcpClientState *state)
{
	int off = 0;
	
	switch (state->msg_type) {
	case TCP_MSG_CID:
		state->client_id = state->read_buf[0];
		printf("Got CID: %d\n", state->client_id);
		break;
	case TCP_MSG_CMDS: /* parse message as chain fo commands */			
		while (off != state->msg_size) {					
			int cmdsize = command_size((Netcmd*)(state->read_buf+off));												
			Netcmd *cmd = malloc(cmdsize);
			memcpy(cmd, state->read_buf+off, cmdsize);
			off += cmdsize;
			
			queue_push(state->in, cmd);				
		}
					
		state->waiting = 0;
		printf("Waited %f %d\n",glfwGetTime()-state->wait_start,state->msg_size);
		break;
	case TCP_MSG_SNAPSHOT:		
		state->snapshot_callback(state->read_buf, state->msg_size);
		printf("Received snapshot. Confirming...\n");
		send_confirmation(state);
		state->ready = 1;
		break;
	default:
		printf("error: no message to process.\n");
	}
	state->msg_type = TCP_MSG_NONE;
}

//-----------------------------------------------------------------------------
static void client_read(TcpClient * socket, char *buf, int len)
{
	TcpClientState *state = tcpclient_get_user_data(socket);	
	
	int i = 0;
	while (i != len) {
		/* reading new msg */
		if (state->msg_size == 0) {
			state->msg_type = buf[i++];													
			state->msg_size = *(uint32_t*)(buf + i);				
			state->read_buf = realloc(state->read_buf, state->msg_size);
			state->num_read_bytes = 0;
			i += sizeof(uint32_t);				
		}
		
		int nbytes = state->msg_size - state->num_read_bytes;		
		int rbytes = (len - i) < nbytes ? (len - i) : nbytes;
		memcpy(state->read_buf + state->num_read_bytes, buf + i, rbytes);		
		i += rbytes;
		state->num_read_bytes += rbytes;		
			
		if (state->num_read_bytes == state->msg_size) {			
			process_message(state);			
			state->msg_size = 0;
			if (i != len) {
				printf("error: more than one message per buffer.\n");
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
NetworkType* new_tcp_client_state(char* ip, int port, void *login_data, uint32_t ldsize, ClientSnapshotCallback cb)
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
	state->ready = 0;
	state->read_buf = malloc(1);

	state->socket = new_tcpclient();
	tcpclient_init(state->socket, port, ip);
	tcpclient_set_user_data(state->socket, tcp->state);

	printf("Connecting...\n");
	tcpclient_set_handlers(state->socket, &client_read, &client_disconnect);
	tcpclient_connect(state->socket);
	
	packet[0] = TCP_MSG_LOGIN;
	memcpy(packet+1, &ldsize, 4);
	memcpy(packet+5, login_data, ldsize);
	tcpclient_write(state->socket, packet, 5+ldsize);
	
	state->snapshot_callback = cb;
	
	while (!state->ready) {
		tick(state);
	}
	
	return tcp;
}

