#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glfw.h>

#include "network.h"
#include "queue.h"
#include "socket.h"
#include "tcp_network.h"

#define MAX_CONNECTIONS 20

typedef struct {
	char *read_buf;
	uint32_t num_read_bytes;
	uint32_t msg_size;
	int msg_type;
	int waiting;
	int needs_greeting;
	int active;
} TcpConnection;

typedef struct
{
	TcpServer *socket;
	char bulk_buf[8196];
	uint32_t bulk_size;
	TcpConnection connections[MAX_CONNECTIONS];
        
	SnapshotCallback snapshot_callback;
	LoginCallback login_callback;	
} TcpServerState;

//-----------------------------------------------------------------------------
static void tick(void *d)
{
	TcpServerState *state = (TcpServerState *) d;
	tcpserver_select(state->socket);
	
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (state->connections[i].needs_greeting) {
			uint32_t data_size = 1;
			state->bulk_buf[0] = TCP_MSG_CID;
			memcpy(state->bulk_buf+1, &data_size, sizeof(uint32_t));
			state->bulk_buf[5] = i;
			tcpserver_write(state->socket, i, state->bulk_buf, 5+data_size);
						
			state->connections[i].needs_greeting = 0;
			state->connections[i].waiting = 1; // this will cause lag when new player joins
		}
	}
	
	int none_waits = 1;
	for (int i = 0; i< MAX_CONNECTIONS; i++) {
		none_waits &= !(state->connections[i].active && state->connections[i].waiting);
	}
			
	if (none_waits) {		
		uint32_t data_size = state->bulk_size - 5;
		state->bulk_buf[0] = TCP_MSG_CMDS;
		memcpy(state->bulk_buf+1, &data_size, sizeof(uint32_t));			
		for (int i = 0; i< MAX_CONNECTIONS; i++) {				
			if (state->connections[i].active) {				
				tcpserver_write(state->socket, i, state->bulk_buf, state->bulk_size);
				state->connections[i].waiting = 1;				
			}
		}		
		state->bulk_size = 5;
	}
}

//-----------------------------------------------------------------------------
static void logic_tick(void *d)
{	
}

//-----------------------------------------------------------------------------
static void add_command(void *d, Netcmd *cmd)
{	
}

//-----------------------------------------------------------------------------
static Netcmd *get_command(void *d)
{
	return NULL;
}

//-----------------------------------------------------------------------------
static void cleanup(void *d)
{	
}

//-----------------------------------------------------------------------------
static int server_accept(TcpServer *server, int cid)
{
	TcpServerState *state = tcpserver_get_user_data(server);
	state->connections[cid].active = 1;
	state->connections[cid].msg_size = 0;
	state->connections[cid].waiting = 0;
	state->connections[cid].needs_greeting = 1;
	printf("Client %d connected...\n", cid);
	return 1;
}

static void server_disconnect(TcpServer *server, int cid, int gracefully)
{
	TcpServerState *state = tcpserver_get_user_data(server);
	state->connections[cid].active = 0;
	printf("Client %d left gracefully:%d...\n", cid, gracefully);
}

//-----------------------------------------------------------------------------
static void send_snapshot(TcpServerState *state, int cid)
{
	void *snapshot;
	uint32_t sshot_size;
	state->snapshot_callback(&snapshot, cid, &sshot_size);
	state->bulk_buf[0] = TCP_MSG_SNAPSHOT;
	memcpy(state->bulk_buf+1, &sshot_size, 4);
	memcpy(state->bulk_buf+5, snapshot, sshot_size);
	tcpserver_write(state->socket, cid, state->bulk_buf, 5+sshot_size);
	free(snapshot);
}

//-----------------------------------------------------------------------------
static void process_message(TcpServerState *state, int cid)
{
	TcpConnection *conn = &state->connections[cid];
	
	switch (conn->msg_type) {
	case TCP_MSG_CMDS:
		memcpy(state->bulk_buf + state->bulk_size, conn->read_buf, conn->msg_size);
		state->bulk_size += conn->msg_size;			
		conn->waiting = 0;
		break;
	case TCP_MSG_CONFIRM:
		printf("CID %d confirmed snapshot.\n", cid);
		conn->waiting = 0;
		break;
	case TCP_MSG_LOGIN:		 
		if (state->login_callback(conn->read_buf, cid, conn->msg_size)) {
			printf("CID %d logged in. Sending recent snapshot.\n", cid);
			send_snapshot(state, cid);
		}		
		break;
	default:		
		printf("error: no message to process.\n");
	}			
}

//-----------------------------------------------------------------------------
static void server_read(TcpServer *server, int cid, char *buf, int len)
{
	TcpServerState *state = tcpserver_get_user_data(server);
	TcpConnection *conn = &state->connections[cid];
	
	int i = 0;
	while (i != len) {
		if (conn->msg_size == 0) {
			conn->msg_type = buf[i++];
			conn->msg_size = *(uint32_t*)(buf + i);
			conn->read_buf = realloc(conn->read_buf, conn->msg_size);
			conn->num_read_bytes = 0;
			i += sizeof(uint32_t);
		}
		
		int nbytes = conn->msg_size - conn->num_read_bytes;
		int rbytes = (len - i) < nbytes ? (len - i) : nbytes;
		memcpy(conn->read_buf + conn->num_read_bytes, buf + i, rbytes);
		i += rbytes;
		conn->num_read_bytes += rbytes;
		
		if (conn->num_read_bytes == conn->msg_size) {
			process_message(state, cid);			
			conn->msg_size = 0;
			conn->msg_type = TCP_MSG_NONE;
		}
	}
}

//-----------------------------------------------------------------------------
void tcpserverstate_set_snapshot_callback(void *s, SnapshotCallback cb)
{
	TcpServerState *state = (TcpServerState *) s;
	state->snapshot_callback = cb;
}
	
//-----------------------------------------------------------------------------
void tcpserverstate_set_login_callback(void *s, LoginCallback cb)
{
	TcpServerState *state = (TcpServerState *) s;
	state->login_callback = cb;
}


//-----------------------------------------------------------------------------
NetworkType *new_tcp_server_state(void)
{
	NetworkType *tcp = malloc(sizeof(NetworkType));
	tcp->tick = tick;
	tcp->logic_tick = logic_tick;
	tcp->add_command = add_command;
	tcp->get_command = get_command;
	tcp->cleanup = cleanup;

	TcpServerState *state = (TcpServerState *) malloc(sizeof(TcpServerState));
	tcp->state = (void *)state;
	
	state->bulk_size = 5;
	
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		state->connections[i].read_buf = malloc(1);
	}

	state->socket = new_tcpserver();
	tcpserver_init(state->socket, 1234);
	tcpserver_set_user_data(state->socket, tcp->state);
	
	tcpserver_set_handlers(state->socket, &server_read, &server_accept, &server_disconnect);
	tcpserver_listen(state->socket);
	printf("Server ready...\n");
	
	return tcp;
}

