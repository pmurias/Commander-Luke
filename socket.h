#ifndef __SOCKET_H__
#define __SOCKET_H__

struct _SockAddr;
typedef struct _SockAddr SockAddr;
struct _SockAddrs;
typedef struct _SockAddrs SockAddrs;
struct _Socket;
typedef struct _Socket UdpSocket;
struct _TcpServer;
typedef struct _TcpServer TcpServer;
struct _TcpClient;
typedef struct _TcpClient TcpClient;



typedef void (*TcpServerReadHandler)(TcpServer *, int, char *, int);
typedef int (*TcpServerAcceptHandler)(TcpServer *, int);
typedef void (*TcpServerDisconnectHandler)(TcpServer *, int, int);
typedef void (*TcpClientReadHandler)(TcpClient *, char *, int);
typedef void (*TcpClientDisconnectHandler)(TcpClient *);

void socket_startup();
void socket_cleanup();

TcpServer *new_tcpserver();
void tcpserver_init(TcpServer *server, int port);
void tcpserver_listen(TcpServer *server);
void tcpserver_select(TcpServer *server);
void tcpserver_write(TcpServer *server, int connection, char *buf, int len);
void tcpserver_set_handlers(TcpServer *server, 
	TcpServerReadHandler readHandler, TcpServerAcceptHandler acceptHandler, TcpServerDisconnectHandler disconnectHandler);
void tcpserver_close_connection(TcpServer *server, int connection);

TcpClient *new_tcpclient();
void tcpclient_init(TcpClient *client, int servPort, char *servIp);
int tcpclient_connect(TcpClient *client);
void tcpclient_select(TcpClient *client);
void tcpclient_write(TcpClient *client, char *buf, int len);
void tcpclient_set_handlers(TcpClient *client, TcpClientReadHandler readHandler, TcpClientDisconnectHandler disconnectHandler);
void tcpclient_close(TcpClient *client);
int tcpclient_is_connected(TcpClient *client);

#endif // __SOCKET_H__