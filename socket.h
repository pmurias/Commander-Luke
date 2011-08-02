#ifndef __SOCKET_H__
#define __SOCKET_H__

struct _Socket;
typedef struct _Socket Socket;
struct _ServerSocket;
typedef struct _ServerSocket ServerSocket;
struct _ClientSocket;
typedef struct _ClientSocket ClientSocket;

typedef void (*ServerReadHandler)(ServerSocket *, Socket *, char *, int);
typedef int (*ServerAcceptHandler)(ServerSocket *, Socket *);
typedef void (*ClientReadHandler)(ClientSocket *, char *, int);

void socket_startup();
void socket_cleanup();

ServerSocket *new_serversocket();
void serversocket_init(ServerSocket *server, int port);
void serversocket_listen(ServerSocket *server);
void serversocket_select(ServerSocket *server);
int serversocket_get_connection(ServerSocket *server, Socket *socket);
void serversocket_write(ServerSocket *server, int connection, char *buf, int len);
void serversocket_set_handlers(ServerSocket *server, ServerReadHandler readHandler, ServerAcceptHandler acceptHandler);

ClientSocket *new_clientsocket();
void clientsocket_init(ClientSocket *client, int servPort, char *servIp);
void clientsocket_connect(ClientSocket *client);
void clientsocket_select(ClientSocket *client);
void clientsocket_write(ClientSocket *client, char *buf, int len);
void clientsocket_set_handlers(ClientSocket *client, ClientReadHandler readHandler);

#endif // __SOCKET_H__