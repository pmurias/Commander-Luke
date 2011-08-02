#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef WIN32
	#define _WIN32_WINNT 0x0501
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <netinet/ip.h>
	#include <arpa/inet.h>

	typedef int SOCKET;
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define closesocket(s) close(s);
#endif

#include "socket.h"

#ifndef ZeroMemory
#define ZeroMemory(a, c) (memset((a), 0, (c)))
#endif

struct _Socket
{
	SOCKET handle;
	struct sockaddr_in addr;
	int addr_len;
};

#define SOCK_BUFFER_LEN 8
#define MAX_CLIENTS 64

struct _ServerSocket
{
	Socket *socket;		
	Socket *clients[MAX_CLIENTS];
	char write_buffers[MAX_CLIENTS][SOCK_BUFFER_LEN];
	int num_write_bytes[MAX_CLIENTS];
	fd_set read_set;
	fd_set write_set;
	fd_set exc_set;
	ServerReadHandler read_handler;
	ServerAcceptHandler accept_handler;
};

struct _ClientSocket
{
	Socket *socket;
	char write_buffer[SOCK_BUFFER_LEN];
	int num_write_bytes;
	fd_set read_set;
	fd_set write_set;
	fd_set exc_set;
	ClientReadHandler read_handler;
};

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
Socket *new_socket()
{
	Socket *sock = malloc(sizeof(Socket));
	ZeroMemory(sock, sizeof(Socket));
	
	sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock->handle == INVALID_SOCKET) {
		perror("Socket error: Cannot create socket.\n");
		exit(1);
	}
	sock->addr_len = sizeof(sock->addr);
	ZeroMemory(&sock->addr, sizeof(sock->addr));
	return sock;
}

//-----------------------------------------------------------------------------
void socket_non_block(Socket *socket)
{
	#ifndef WIN32
	int flags = fcntl(socket->handle, F_GETFL, 0);
	fcntl(socket->handle, F_SETFL, flags | O_NONBLOCK);
	#else
	unsigned long iMode=1;
	ioctlsocket(socket->handle,FIONBIO,&iMode);
	#endif
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
	
	client->num_write_bytes = 0;
}

//-----------------------------------------------------------------------------
void clientsocket_connect(ClientSocket *client)
{
	if (client->read_handler == NULL) {
		printf("Socket error: Read handler not given.\n");
		exit(1);
	}
	
	if (connect(client->socket->handle, (struct sockaddr*)&client->socket->addr, client->socket->addr_len) == SOCKET_ERROR) {
		printf("Socket error: Could not connect.\n");
		exit(1);
	}
	socket_non_block(client->socket);
}

//-----------------------------------------------------------------------------
void clientsocket_init_sets(ClientSocket *client)
{
	FD_ZERO(&client->read_set);
	FD_ZERO(&client->write_set);
	FD_ZERO(&client->exc_set);
	
	FD_SET(client->socket->handle, &client->read_set);
	if (client->num_write_bytes > 0) {
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
	
	if (select(0, &client->read_set, &client->write_set, &client->exc_set, &tv) > 0) {
		if (FD_ISSET(client->socket->handle, &client->read_set)) {			
			ZeroMemory(readBuffer, SOCK_BUFFER_LEN);
			int num_bytes_read = recv(client->socket->handle, readBuffer, SOCK_BUFFER_LEN, 0);
			if (num_bytes_read == SOCKET_ERROR || num_bytes_read == 0) 
			{
				if (num_bytes_read == 0) {
					/* disconnection handler */
				} else {
					printf("Socket error: server closed connection unexpectedly.\n");
				}
			} else {
				client->read_handler(client, readBuffer, num_bytes_read);
			}
		}
		
		if (FD_ISSET(client->socket->handle, &client->write_set))
		{
			int num_bytes_writen = send(client->socket->handle,
			client->write_buffer, client->num_write_bytes, 0);
				
			if (num_bytes_writen != client->num_write_bytes) {
				printf("Socket error: didn't manage to send whole buffer.\n");
			}
				
			client->num_write_bytes = 0;
		}
			
		if (FD_ISSET(client->socket->handle, &client->exc_set)) {
			printf("Socket error: client exception.\n");
		}
	}
}

//-----------------------------------------------------------------------------
void clientsocket_write(ClientSocket *client, char *buf, int len)
{	
	int sendLen = len < SOCK_BUFFER_LEN ? len : SOCK_BUFFER_LEN;
	memcpy(client->write_buffer, buf, sendLen);
	client->num_write_bytes = sendLen;
}

//-----------------------------------------------------------------------------
void clientsocket_set_handlers(ClientSocket *client, ClientReadHandler readHandler)
{
	client->read_handler = readHandler;
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
		perror("Socket error: Bind error.\n");
		closesocket(server->socket->handle);
		exit(1);
	}
	
	ZeroMemory(server->clients, sizeof(Socket*)*MAX_CLIENTS);
	ZeroMemory(server->num_write_bytes, sizeof(int)*MAX_CLIENTS);
	server->read_handler = NULL;
	server->accept_handler = NULL;
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
			if (server->num_write_bytes[i] > 0)
				FD_SET(server->clients[i]->handle, &server->write_set);
			else
				FD_SET(server->clients[i]->handle, &server->read_set);
				
			FD_SET(server->clients[i]->handle, &server->exc_set);
		}
	}
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
	
	if (select(0, &server->read_set, &server->write_set, &server->exc_set, &tv) > 0)  
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
						server->num_write_bytes[i] = 0;						
						break;
					}
				}				
			}
			/* else refuse connection */
		}
		
		if (FD_ISSET(server->socket->handle, &server->exc_set)) {
			printf("Socket error: Server exception.\n");
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
					if (num_bytes_read == 0) {
						/* disconnection handler */
					} else {					
						printf("Socket error: client closed connection unexpectedly.\n");
					}
					/* close connection */
					free(server->clients[i]);
					server->clients[i] = NULL;
					continue;
				} else {
					server->read_handler(server, server->clients[i], readBuffer, num_bytes_read);
				}
			}
			
			if (FD_ISSET(server->clients[i]->handle, &server->write_set))
			{
				int num_bytes_writen = send(server->clients[i]->handle,
					server->write_buffers[i], server->num_write_bytes[i], 0);
				
				if (num_bytes_writen != server->num_write_bytes[i]) {
					printf("Socket error: didn't manage to send whole buffer.\n");
				}
				
				server->num_write_bytes[i] = 0;
			}
			
			if (FD_ISSET(server->clients[i]->handle, &server->exc_set))
			{
				printf("Socket error: client exception.\n");
				free(server->clients[i]);
				server->clients[i] = NULL;				
			}
		}
	}
}

//-----------------------------------------------------------------------------
void serversocket_listen(ServerSocket *server)
{
	if (server->read_handler == NULL || server->accept_handler == NULL) {
		printf("Socket error: Read or Accept handler not given.\n");
		exit(1);
	}
	
	if (listen(server->socket->handle, SOMAXCONN) == SOCKET_ERROR) {
		perror("Socket error: Could not listen.\n");
		closesocket(server->socket->handle);
		exit(1);
	}
	socket_non_block(server->socket);
}

//-----------------------------------------------------------------------------
void serversocket_set_handlers(ServerSocket *server, ServerReadHandler readHandler, ServerAcceptHandler acceptHandler)
{
	server->read_handler = readHandler;
	server->accept_handler = acceptHandler;
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
	int sendLen = len < SOCK_BUFFER_LEN ? len : SOCK_BUFFER_LEN;	
	memcpy(server->write_buffers[connection], buf, sendLen);
	server->num_write_bytes[connection] = sendLen;	
}