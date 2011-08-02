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

void server_disconnected(ServerSocket *server, Socket *client, int gracefully)
{
	printf("Client disconnected %s\n", gracefully ? "gracefully" : "");
}

void client_read(ClientSocket *client, char *buf, int len)
{
	printf("Server replied: %s, len %d\n", buf, len);	
}

void client_disconnected(ClientSocket *client)
{
	printf("Disonected handle\n");
}

int main(int argc, char **argv)
{
	socket_startup();
	
	if (argc == 2) {
	
	
		if (strcmp(argv[1], "--server") == 0) {
			ServerSocket *servor = new_serversocket();		
			serversocket_init(servor, 1234);			
			serversocket_set_handlers(servor, &read_handler, &accept_handler, &server_disconnected);
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
			clientsocket_set_handlers(client, &client_read, &client_disconnected);
			clientsocket_connect(client);
			char buf[255];
			int cnt = 300/8;
			int msgs=130;
			while (msgs > -300) {
				if (clientsocket_is_connected(client)) {
				
					clientsocket_select(client);
					
					cnt--;
				                                  
					if (cnt ==0) {
						 memset(buf, 0, 255);
						
						for (int i = 0; i < 250; i++)
							buf[i] = 'a' + (rand()%('z'-'a'));
						buf[250] = 0;
						clientsocket_write(client, buf, strlen(buf));				                                       
						cnt = 300/8;
					}
					msgs--;
					if (msgs==0) {											
						for (int i = 0; i <100; i++) { printf("\t"); clientsocket_select(client);	}
						clientsocket_close(client);
					}
				
				} else {	
					printf("Trying to reconnect...\n");
					if (!clientsocket_connect(client))
						return 0;
				}												
			}
			for (int i = 0; i <100; i++) { printf("\t");  clientsocket_select(client);	}
			clientsocket_close(client);
		}
	}
	
	socket_cleanup();
	return 0;
}
