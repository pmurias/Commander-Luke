#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef WIN32
	#define _WIN32_WINNT 0x0501
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <netinet/ip.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>

	#define SOCKET int
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define closesocket(s) close(s);
#endif

#include "socket.h"

#ifndef ZeroMemory
#define ZeroMemory(a, c) (memset((a), 0, (c)))
#endif

#define SOCK_BUFFER_LEN 8
#define MAX_CLIENTS 64

/* socket buffer is single linked list */
typedef struct _SocketBufferChunk
{
	char data[SOCK_BUFFER_LEN];
	int num_bytes;
	struct _SocketBufferChunk *next;
} SocketBufferChunk;

typedef struct _SocketBuffer
{
	SocketBufferChunk *first_chunk;
	SocketBufferChunk *last_chunk;
} SocketBuffer;

struct _Socket
{
	SOCKET handle;
	struct sockaddr_in addr;
	socklen_t addr_len;
};

struct _ServerSocket
{
	Socket *socket;		
	Socket *clients[MAX_CLIENTS];
	SocketBuffer write_buffers[MAX_CLIENTS];
	fd_set read_set;
	fd_set write_set;
	fd_set exc_set;
	ServerReadHandler read_handler;
	ServerAcceptHandler accept_handler;
	ServerDisconnectHandler disconnect_handler;
};

struct _ClientSocket
{
	Socket *socket;
	SocketBuffer write_buffer;	
	fd_set read_set;
	fd_set write_set;
	fd_set exc_set;
	ClientReadHandler read_handler;
	ClientDisconnectHandler disconnect_handler;
	int is_connected;
};

//-----------------------------------------------------------------------------
static SocketBufferChunk *new_socketbufferchunk(char *buf, int len)
{
	SocketBufferChunk *sb = malloc(sizeof(SocketBufferChunk));
	memcpy(sb->data, buf, len);
	sb->num_bytes = len;
	sb->next = NULL;
	return sb;
}

//-----------------------------------------------------------------------------
static void socketbuffer_load_data(SocketBuffer *buffer, char *data, int len)
{
	int sendLen;
	SocketBufferChunk *newChunk;
	
	while (len > 0) 
	{
		sendLen = len < SOCK_BUFFER_LEN ? len : SOCK_BUFFER_LEN;
		newChunk = new_socketbufferchunk(data, sendLen);
		if (buffer->last_chunk == NULL) {
			buffer->first_chunk = buffer->last_chunk = newChunk;
		} else {
			buffer->first_chunk->next = newChunk;
			buffer->first_chunk = newChunk;
		}
		len -= sendLen;
		data += sendLen;
	}
}

//-----------------------------------------------------------------------------
static void socketbuffer_free(SocketBuffer *buffer)
{
	SocketBufferChunk *next, *chunk = buffer->last_chunk;
	while (chunk != NULL) {
		next = chunk->next;
		free(chunk);
		chunk = next;
	}
	buffer->first_chunk = buffer->last_chunk = NULL;
}

//-----------------------------------------------------------------------------
void socket_startup()
{
	#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1,1), &wsaData);	
	#endif
}

//-----------------------------------------------------------------------------
void socket_cleanup()
{
	#ifdef WIN32
	WSACleanup();
	#endif
}

//-----------------------------------------------------------------------------
static Socket *new_socket()
{
	Socket *sock = malloc(sizeof(Socket));
	ZeroMemory(sock, sizeof(Socket));
	
	sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock->handle == INVALID_SOCKET) {
		printf("Socket error: Cannot create socket.\n");
		exit(1);
	}
	sock->addr_len = sizeof(sock->addr);
	ZeroMemory(&sock->addr, sizeof(sock->addr));
	return sock;
}

//-----------------------------------------------------------------------------
static void socket_non_block(Socket *socket)
{
	#ifndef WIN32
	int flags = fcntl(socket->handle, F_GETFL, 0);
	fcntl(socket->handle, F_SETFL, flags | O_NONBLOCK);
	#else
	unsigned long iMode=1;
	ioctlsocket(socket->handle,FIONBIO,&iMode);
	#endif
	/* speed up small packet transmission by disabling Nagel */
	char optval;
	setsockopt(socket->handle, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(int));
}

//-----------------------------------------------------------------------------
ClientSocket *new_clientsocket()
{
	ClientSocket *client = malloc(sizeof(ClientSocket));
	return client;
}

//-----------------------------------------------------------------------------
void clientsocket_init(ClientSocket *client, int servPort, char *servIp)
{	
	client->socket = new_socket();
	client->socket->addr.sin_family = AF_INET;
	client->socket->addr.sin_port = htons(servPort);
	struct addrinfo *res;
	getaddrinfo(servIp, NULL, NULL, &res);
	struct sockaddr_in *ai_addr = (struct sockaddr_in *)res->ai_addr;
	client->socket->addr.sin_addr.s_addr = ai_addr->sin_addr.s_addr;	
	
	client->write_buffer.first_chunk = NULL;
	client->write_buffer.last_chunk = NULL;
	client->read_handler = NULL;
	client->disconnect_handler = NULL;
	client->is_connected = 0;
}

//-----------------------------------------------------------------------------
int clientsocket_connect(ClientSocket *client)
{
	if (client->read_handler == NULL) {
		printf("Socket error: Read handler not given.\n");
		return 0;
	}
	
	if (connect(client->socket->handle, (struct sockaddr*)&client->socket->addr, client->socket->addr_len) == SOCKET_ERROR) {
		printf("Socket error: Could not connect.\n");
		return 0;
	}
	socket_non_block(client->socket);
	client->is_connected = 1;
	return 1;
}

//-----------------------------------------------------------------------------
void clientsocket_init_sets(ClientSocket *client)
{
	FD_ZERO(&client->read_set);
	FD_ZERO(&client->write_set);
	FD_ZERO(&client->exc_set);
	
	FD_SET(client->socket->handle, &client->read_set);
	if (client->write_buffer.last_chunk != NULL) {
		FD_SET(client->socket->handle, &client->write_set);
	}
	FD_SET(client->socket->handle, &client->exc_set);
}

//-----------------------------------------------------------------------------
void clientsocket_select(ClientSocket *client)
{
	static char readBuffer[SOCK_BUFFER_LEN];
	clientsocket_init_sets(client);	
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	
	if (select(FD_SETSIZE, &client->read_set, &client->write_set, &client->exc_set, &tv) > 0) {
		if (FD_ISSET(client->socket->handle, &client->read_set)) {			
			ZeroMemory(readBuffer, SOCK_BUFFER_LEN);
			int num_bytes_read = recv(client->socket->handle, readBuffer, SOCK_BUFFER_LEN, 0);
			if (num_bytes_read == SOCKET_ERROR || num_bytes_read == 0) 
			{
				if (num_bytes_read != 0) {								
					printf("Socket error: server closed connection unexpectedly.\n");
				}
				
				socketbuffer_free(&client->write_buffer);
				client->is_connected = 0;
				
				if (client->disconnect_handler) {
					client->disconnect_handler(client);
				}
				return;				
			} else {
				client->read_handler(client, readBuffer, num_bytes_read);
			}
		}
		
		if (FD_ISSET(client->socket->handle, &client->write_set))
		{
			SocketBufferChunk *chunk = client->write_buffer.last_chunk;
			int num_bytes_writen = send(client->socket->handle,
			chunk->data, chunk->num_bytes, 0);
				
			if (num_bytes_writen != chunk->num_bytes) {
				printf("Socket error: didn't manage to send whole buffer.\n");
			}
				
			client->write_buffer.last_chunk = chunk->next;
			free(chunk);
		}
			
		if (FD_ISSET(client->socket->handle, &client->exc_set)) {
			printf("Socket error: client exception.\n");
			socketbuffer_free(&client->write_buffer);
			client->is_connected = 0;
		}
	}
}

//-----------------------------------------------------------------------------
void clientsocket_write(ClientSocket *client, char *buf, int len)
{	
	socketbuffer_load_data(&client->write_buffer, buf, len);
}

//-----------------------------------------------------------------------------
void clientsocket_set_handlers(ClientSocket *client, ClientReadHandler readHandler, ClientDisconnectHandler disconnectHandler)
{
	client->read_handler = readHandler;
	client->disconnect_handler = disconnectHandler;
}

//-----------------------------------------------------------------------------
void clientsocket_close(ClientSocket *client)
{
	shutdown(client->socket->handle, SD_SEND);
	closesocket(client->socket->handle);
	socketbuffer_free(&client->write_buffer);
	client->is_connected = 0;
	
	/* we have to get new handle, because closesocket() destroyed previous one */
	client->socket->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client->socket->handle == INVALID_SOCKET) {
		printf("Socket error: Cannot recreate socket.\n");
		exit(1);
	}
}

//-----------------------------------------------------------------------------
int clientsocket_is_connected(ClientSocket *client)
{
	return client->is_connected;
}

//-----------------------------------------------------------------------------
ServerSocket *new_serversocket()
{
	ServerSocket *server = malloc(sizeof(ServerSocket));
	return server;
}

//-----------------------------------------------------------------------------
void serversocket_init(ServerSocket *server, int port)
{			
	server->socket = new_socket();
	server->socket->addr.sin_family = AF_INET;
	server->socket->addr.sin_port = htons(port);
	server->socket->addr.sin_addr.s_addr = htonl(INADDR_ANY);	
		
	if (bind(server->socket->handle, (struct sockaddr*)&server->socket->addr, server->socket->addr_len) == SOCKET_ERROR) {
		printf("Socket error: Bind error.\n");
		closesocket(server->socket->handle);
		exit(1);
	}
	
	ZeroMemory(server->clients, sizeof(Socket*)*MAX_CLIENTS);
	ZeroMemory(server->write_buffers, sizeof(SocketBuffer)*MAX_CLIENTS);
	server->read_handler = NULL;
	server->accept_handler = NULL;
	server->disconnect_handler = NULL;
}

//-----------------------------------------------------------------------------
void serversocket_init_sets(ServerSocket *server)
{
	FD_ZERO(&server->read_set);
	FD_ZERO(&server->write_set);
	FD_ZERO(&server->exc_set);
	
	FD_SET(server->socket->handle, &server->read_set);
	FD_SET(server->socket->handle, &server->write_set);
	FD_SET(server->socket->handle, &server->exc_set);
	
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (server->clients[i]) 
		{
			if (server->write_buffers[i].last_chunk != NULL)
				FD_SET(server->clients[i]->handle, &server->write_set);
			else
				FD_SET(server->clients[i]->handle, &server->read_set);
				
			FD_SET(server->clients[i]->handle, &server->exc_set);
		}
	}
}

//-----------------------------------------------------------------------------
static void serversocket_free_connection(ServerSocket *server, int c)
{	
	socketbuffer_free(&server->write_buffers[c]);
	free(server->clients[c]);
	server->clients[c] = NULL;
}

//-----------------------------------------------------------------------------
void serversocket_select(ServerSocket *server)
{
	static char readBuffer[SOCK_BUFFER_LEN];
	serversocket_init_sets(server);
	Socket *client = NULL;	
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	
	if (select(FD_SETSIZE, &server->read_set, &server->write_set, &server->exc_set, &tv) > 0)  
	{	
		/* new connection is waiting for accept */
		if (FD_ISSET(server->socket->handle, &server->read_set)) 
		{                        
			client = new_socket();
			client->handle = accept(server->socket->handle, (struct sockaddr*)&client->addr, &client->addr_len);
			if (client->handle == INVALID_SOCKET) {
				printf("Socket error: Could not accept connection.\n");
			}
			
			if (server->accept_handler(server, client)) 
			{
				socket_non_block(client);				
				for (int i=0; i<MAX_CLIENTS; ++i) {
					if (server->clients[i] == NULL) {
						server->clients[i] = client;
						break;
					}
				}				
			}
			/* else refuse connection */
		}
		
		if (FD_ISSET(server->socket->handle, &server->exc_set)) {
			printf("Socket: Server exception.\n");
			return;
		}
		
		/* now handle clients read and write */
		for (int i=0; i<MAX_CLIENTS; ++i) {
			if (server->clients[i] == NULL ||  server->clients[i] == client)
				continue;
				
			if (FD_ISSET(server->clients[i]->handle, &server->read_set)) 
			{
				ZeroMemory(readBuffer, SOCK_BUFFER_LEN);
				int num_bytes_read = recv(server->clients[i]->handle, readBuffer, SOCK_BUFFER_LEN, 0);
				if (num_bytes_read == SOCKET_ERROR || num_bytes_read == 0) 
				{
					if (num_bytes_read != 0) {
						printf("Socket: client closed connection unexpectedly.\n");						
					}
					
					if (server->disconnect_handler) {
						server->disconnect_handler(server, server->clients[i], num_bytes_read == 0);
					}
					
					serversocket_free_connection(server, i);
					continue;
				} else {
					server->read_handler(server, server->clients[i], readBuffer, num_bytes_read);
				}
			}
			
			if (FD_ISSET(server->clients[i]->handle, &server->write_set))
			{
				SocketBufferChunk *chunk = server->write_buffers[i].last_chunk;
				int num_bytes_writen = send(server->clients[i]->handle,
					chunk->data, chunk->num_bytes, 0);
				
				if (num_bytes_writen != chunk->num_bytes) {
					printf("Socket: didn't manage to send whole buffer.\n");
				}								
				
				server->write_buffers[i].last_chunk = chunk->next;
				free(chunk);
			}
			
			if (FD_ISSET(server->clients[i]->handle, &server->exc_set))
			{
				printf("Socket: client exception.\n");
				serversocket_free_connection(server, i);		
			}
		}
	}
}

//-----------------------------------------------------------------------------
void serversocket_listen(ServerSocket *server)
{
	if (server->read_handler == NULL || server->accept_handler == NULL) {
		printf("Socket: Read or Accept handler not given.\n");
		exit(1);
	}
	
	if (listen(server->socket->handle, SOMAXCONN) == SOCKET_ERROR) {
		printf("Socket: Could not listen.\n");
		closesocket(server->socket->handle);
		exit(1);
	}
	socket_non_block(server->socket);
}

//-----------------------------------------------------------------------------
void serversocket_set_handlers(ServerSocket *server, 
	ServerReadHandler readHandler, ServerAcceptHandler acceptHandler, ServerDisconnectHandler disconnectHandler)
{
	server->read_handler = readHandler;
	server->accept_handler = acceptHandler;
	server->disconnect_handler = disconnectHandler;
}

//-----------------------------------------------------------------------------
int serversocket_get_connection(ServerSocket *server, Socket *client)
{
	int i;
	for (i = 0; i<MAX_CLIENTS; ++i) {
		if (server->clients[i] == client) {
			break;
		}
	}
	return i != MAX_CLIENTS ? i : -1;
}

//-----------------------------------------------------------------------------
void serversocket_write(ServerSocket *server, int connection, char *buf, int len)
{	
	socketbuffer_load_data(&server->write_buffers[connection], buf, len);
}

//-----------------------------------------------------------------------------
void serversocket_close_connection(ServerSocket *server, int connection)
{
	shutdown(server->clients[connection]->handle, SD_SEND);
	closesocket(server->clients[connection]->handle);
	serversocket_free_connection(server, connection);
}