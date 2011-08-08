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
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>

	#define SOCKET int
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define SD_SEND SHUT_WR
	#define closesocket(s) close(s);
#endif

#include "socket.h"

#ifndef ZeroMemory
#define ZeroMemory(a, c) (memset((a), 0, (c)))
#endif

#define SOCK_BUFFER_LEN 1280
#define MAX_CLIENTS 64

struct _SockAddr
{
	struct sockaddr_in addr;
	socklen_t len;
};

/* socket buffer is single linked list */
typedef struct _SocketBufferChunk
{
	char data[SOCK_BUFFER_LEN];
	int num_bytes;
	struct _SocketBufferChunk *next;
} SocketBufferChunk;

typedef struct
{
	SocketBufferChunk *first_chunk;
	SocketBufferChunk *last_chunk;
} SocketBuffer;

typedef struct _Socket
{
	SOCKET handle;
	SockAddr addr;
} Socket;

struct _TcpServer
{
	Socket *socket;		
	Socket *clients[MAX_CLIENTS];
	SocketBuffer write_buffers[MAX_CLIENTS];
	fd_set read_set;
	fd_set write_set;
	fd_set exc_set;
	TcpServerReadHandler read_handler;
	TcpServerAcceptHandler accept_handler;
	TcpServerDisconnectHandler disconnect_handler;
};

struct _TcpClient
{
	Socket *socket;
	SocketBuffer write_buffer;	
	fd_set read_set;
	fd_set write_set;
	fd_set exc_set;
	TcpClientReadHandler read_handler;
	TcpClientDisconnectHandler disconnect_handler;
	int is_connected;
	void *user_data;
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
static Socket *new_tcpsocket()
{
	Socket *sock = malloc(sizeof(Socket));
	ZeroMemory(sock, sizeof(Socket));
	
	sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock->handle == INVALID_SOCKET) {
		printf("Socket error: Cannot create socket.\n");
		exit(1);
	}
	sock->addr.len = sizeof(sock->addr.addr);
	ZeroMemory(&sock->addr.addr, sizeof(sock->addr.addr));
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
}

//-----------------------------------------------------------------------------
static void socket_no_delay(Socket *socket)
{
	int optval = 1;
	setsockopt(socket->handle, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(int));
}

//-----------------------------------------------------------------------------
static void sockaddr_from_ip(SockAddr *addr, char *ip, int port)
{
	ZeroMemory(&addr->addr, sizeof(addr->addr));
	addr->addr.sin_family = AF_INET;
	addr->addr.sin_port = htons(port);	
	struct addrinfo *res;
	getaddrinfo(ip, NULL, NULL, &res);
	struct sockaddr_in *ai_addr = (struct sockaddr_in *)res->ai_addr;
	addr->addr.sin_addr.s_addr = ai_addr->sin_addr.s_addr;	
}

//-----------------------------------------------------------------------------
int sockaddr_cmp(SockAddr *a, SockAddr *b) {
	return a->len == b->len && memcmp(a, b, a->len) == 0;
}

//-----------------------------------------------------------------------------
TcpClient *new_tcpclient()
{
	TcpClient *client = malloc(sizeof(TcpClient));
	return client;
}

//-----------------------------------------------------------------------------
void tcpclient_init(TcpClient *client, int servPort, char *servIp)
{	
	client->socket = new_tcpsocket();
	sockaddr_from_ip(&client->socket->addr, servIp, servPort);
	
	client->write_buffer.first_chunk = NULL;
	client->write_buffer.last_chunk = NULL;
	client->read_handler = NULL;
	client->disconnect_handler = NULL;
	client->is_connected = 0;

#ifndef WIN32
        int optval = 1;
        setsockopt(client->socket->handle,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
#endif

}

//-----------------------------------------------------------------------------
int tcpclient_connect(TcpClient *client)
{
	if (client->read_handler == NULL) {
		printf("Socket error: Read handler not given.\n");
		return 0;
	}
	
	if (connect(client->socket->handle, (struct sockaddr*)&client->socket->addr.addr, client->socket->addr.len) == SOCKET_ERROR) {
		printf("Socket error: Could not connect.\n");
		return 0;
	}
	socket_non_block(client->socket);
	socket_no_delay(client->socket);
	client->is_connected = 1;
	return 1;
}

//-----------------------------------------------------------------------------
void tcpclient_init_sets(TcpClient *client)
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
void tcpclient_select(TcpClient *client)
{
	static char readBuffer[SOCK_BUFFER_LEN];
	tcpclient_init_sets(client);	
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
void tcpclient_write(TcpClient *client, char *buf, int len)
{	
	socketbuffer_load_data(&client->write_buffer, buf, len);
}

//-----------------------------------------------------------------------------
void tcpclient_set_handlers(TcpClient *client, TcpClientReadHandler readHandler, TcpClientDisconnectHandler disconnectHandler)
{
	client->read_handler = readHandler;
	client->disconnect_handler = disconnectHandler;
}

//-----------------------------------------------------------------------------
void tcpclient_close(TcpClient *client)
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
int tcpclient_is_connected(TcpClient *client)
{
	return client->is_connected;
}

//-----------------------------------------------------------------------------
void tcpclient_set_user_data(TcpClient *client, void *userdata)
{
	client->user_data = userdata;
}

//-----------------------------------------------------------------------------
void *tcpclient_get_user_data(TcpClient *client)
{
	return client->user_data;
}

//-----------------------------------------------------------------------------
TcpServer *new_tcpserver()
{
	TcpServer *server = malloc(sizeof(TcpServer));
	return server;
}

//-----------------------------------------------------------------------------
void tcpserver_init(TcpServer *server, int port)
{			
	server->socket = new_tcpsocket();
	server->socket->addr.addr.sin_family = AF_INET;
	server->socket->addr.addr.sin_port = htons(port);
	server->socket->addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);	
		
	if (bind(server->socket->handle, (struct sockaddr*)&server->socket->addr.addr, server->socket->addr.len) == SOCKET_ERROR) {
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
void tcpserver_init_sets(TcpServer *server)
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
static void tcpserver_free_connection(TcpServer *server, int c)
{	
	socketbuffer_free(&server->write_buffers[c]);
	free(server->clients[c]);
	server->clients[c] = NULL;
}

//-----------------------------------------------------------------------------
void tcpserver_select(TcpServer *server)
{
	static char readBuffer[SOCK_BUFFER_LEN];
	tcpserver_init_sets(server);
	Socket *client = NULL;	
	struct timeval tv;
	
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	
	if (select(FD_SETSIZE, &server->read_set, &server->write_set, &server->exc_set, &tv) > 0)  
	{	
		/* new connection is waiting for accept */
		if (FD_ISSET(server->socket->handle, &server->read_set)) 
		{                        
			client = new_tcpsocket();
			client->handle = accept(server->socket->handle, (struct sockaddr*)&client->addr.addr, &client->addr.len);
			if (client->handle == INVALID_SOCKET) {
				printf("Socket error: Could not accept connection.\n");
			}
			
			int conn = 0;
			for (; conn<MAX_CLIENTS; ++conn) {
				if (server->clients[conn] == NULL) {					
					break;
				}
			}				
			if (conn != MAX_CLIENTS && server->accept_handler(server, conn)) 
			{
				server->clients[conn] = client;
				socket_non_block(client);	
				socket_no_delay(client);
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
						server->disconnect_handler(server, i, num_bytes_read == 0);
					}
					
					tcpserver_free_connection(server, i);
					continue;
				} else {
					server->read_handler(server, i, readBuffer, num_bytes_read);
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
				tcpserver_free_connection(server, i);		
			}
		}
	}
}

//-----------------------------------------------------------------------------
void tcpserver_listen(TcpServer *server)
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
	socket_no_delay(server->socket);
}

//-----------------------------------------------------------------------------
void tcpserver_set_handlers(TcpServer *server, 
	TcpServerReadHandler readHandler, TcpServerAcceptHandler acceptHandler, TcpServerDisconnectHandler disconnectHandler)
{
	server->read_handler = readHandler;
	server->accept_handler = acceptHandler;
	server->disconnect_handler = disconnectHandler;
}

//-----------------------------------------------------------------------------
void tcpserver_write(TcpServer *server, int connection, char *buf, int len)
{	
	socketbuffer_load_data(&server->write_buffers[connection], buf, len);
}

//-----------------------------------------------------------------------------
void tcpserver_close_connection(TcpServer *server, int connection)
{
	shutdown(server->clients[connection]->handle, SD_SEND);
	closesocket(server->clients[connection]->handle);
	tcpserver_free_connection(server, connection);
}





//-----------------------------------------------------------------------------
//-------------------------------U----D----P-----------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static int updPacketSize = 1024;

struct _SockAddrs
{
	SockAddr *clients;
	int *active;
	int max;
};

//-----------------------------------------------------------------------------
SockAddrs *new_sockaddrs(int max)
{
	SockAddrs *sa = malloc(sizeof(SockAddrs));
	sa->clients = malloc(sizeof(SockAddr) * max);
	sa->active = malloc(sizeof(int) * max);
	sa->max = max;	
	ZeroMemory(sa->active, sizeof(int) * max);
	return sa;
}

//-----------------------------------------------------------------------------
int sockaddrs_add(SockAddrs *list, SockAddr *addr)
{
	/* some kind of hashmap here would be better ! */
	for (int i = 0; i < list->max; i++) {
		if (list->active[i] && sockaddr_cmp(addr, &list->clients[i])) {
			return i;
		}
	}
	/* not found, add to list and get id */
	for (int i = 0; i < list->max; i++) {
		if (!list->active[i]) {
			memcpy(&list->clients[i], addr, sizeof(SockAddr));
			list->active[i] = 1;
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
SockAddr *sockaddrs_get(SockAddrs *list, int i)
{
	if (list->active[i])
		return &list->clients[i];
	return NULL;
}

//-----------------------------------------------------------------------------
void udpsocket_set_packet_size(int size)
{
	updPacketSize = size;
}

//-----------------------------------------------------------------------------
UdpSocket *new_udpsocket(char *ip, int port)
{
	UdpSocket *sock = malloc(sizeof(Socket));
	ZeroMemory(sock, sizeof(Socket));
	
	sock->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock->handle == INVALID_SOCKET) {
		printf("Socket error: Cannot create udp socket.\n");
		exit(1);
	}	
	sock->addr.len = sizeof(sock->addr.addr);
	if (ip == NULL) {
		ZeroMemory(&sock->addr, sizeof(sock->addr));		
		sock->addr.addr.sin_family = AF_INET;
		sock->addr.addr.sin_port = htons(port);
		sock->addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);		
		sock->addr.len = sizeof(sock->addr.addr);
	} else {
		sockaddr_from_ip(&sock->addr, ip, port);
	}
	socket_non_block(sock);
	return sock;
}

//-----------------------------------------------------------------------------
void udpsocket_listen(UdpSocket *sock)
{
	if (bind(sock->handle, (struct sockaddr *)&sock->addr.addr, sock->addr.len) == SOCKET_ERROR) {
		printf("Udp error: Cannot bind socket.\n");		
	}	
}

//-----------------------------------------------------------------------------
SockAddr *new_sockaddr()
{
	SockAddr *naddr;
	naddr = malloc(sizeof(SockAddr));
	naddr->len = sizeof(naddr->addr);
	ZeroMemory(&naddr->addr, naddr->len);		
	return naddr;
}

//-----------------------------------------------------------------------------
int udpsocket_read(UdpSocket *sock, char *buf, SockAddr *naddr)
{	
	int num_bytes = recvfrom(sock->handle, buf, updPacketSize, 0, (struct sockaddr *)&naddr->addr, &naddr->len);	
	if (num_bytes == SOCKET_ERROR) {
		//printf("Socket error: Cannot read from udp socket\n");
		return -1;
	};
	return num_bytes;
}

//-----------------------------------------------------------------------------
void udpsocket_write(UdpSocket *sock, char *buf, int len, SockAddr *addr)
{
	int num_bytes = sendto(sock->handle, buf, len, 0, (struct sockaddr *)&addr->addr, addr->len);
	if (num_bytes == SOCKET_ERROR) {
		printf("Socket error: Cannot write to udp socket.\n");		
	}
}

//-----------------------------------------------------------------------------
SockAddr *udpsocket_get_addr(UdpSocket *sock)
{
	return &sock->addr;
}


