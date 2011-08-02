#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"

int lastconnection;
char writebuf[255];


int accept_handler(ServerSocket *server, Socket *client)
{
	printf("Connection accepted!\n");		
	return 1;
}

void read_handler(ServerSocket *server, Socket *client, char *buf, int len)
{
	lastconnection = serversocket_get_connection(server, client);
	
	printf("Recieved data: %s, len %d\n", buf, len);		
	for (int i = 0; i< len; i++) 
		writebuf[i] = buf[len-1-i];
}

void client_read(ClientSocket *client, char *buf, int len)
{
	printf("Server replied: %s, len %d\n", buf, len);	
}

int main(int argc, char **argv)
{
	socket_startup();
	
	if (argc == 2) {
	
	
		if (strcmp(argv[1], "--server") == 0) {
			ServerSocket *servor = new_serversocket();		
			serversocket_init(servor, 1234);			
			serversocket_set_handlers(servor, &read_handler, &accept_handler);
			serversocket_listen(servor);
			while (1) {
				serversocket_select(servor);				
				if (strlen(writebuf) > 0) {					
					serversocket_write(servor, lastconnection, writebuf, strlen(writebuf));
					memset(writebuf, 0, 255);
				}
			}
		}
		
		
		
		else if (strcmp(argv[1], "--client") == 0) {
			ClientSocket *client = new_clientsocket();
			clientsocket_init(client, 1234, "127.0.0.1");
			clientsocket_set_handlers(client, &client_read);
			clientsocket_connect(client);
			while (1) {
				clientsocket_select(client);
				char buf[255];
				memset(buf, 0, 255);
								
				gets(buf);
				if (strlen(buf) > 0)
					clientsocket_write(client, buf, strlen(buf));				
			}			
		}
	}
	
	socket_cleanup();
	return 0;
}