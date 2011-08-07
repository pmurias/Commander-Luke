#include <GL/glfw.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "socket.h"
#include "str.h"

#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))

char sockbuf[1501];
SockAddr *caddr = NULL;

typedef struct
{
	int active;	
	char currentData[1501];
	int psize;
	float lastSendTime;
	int dataId;
	Str *cmd;
} ClientData;

ClientData clients[64];

int tcp_serv_psize = 1024;
int server_accept(TcpServer *server, int conn)
{
	printf("Client %d connected...\n", conn);	
	return 1;
}

void server_read(TcpServer *server, int conn, char *buf, int len)
{
	int i = 0;	
	while (i < len) {
		int nbytes = tcp_serv_psize - clients[conn].cmd->len;
		int abytes = len - i;
		int rbytes = abytes<nbytes ? abytes:nbytes;
		str_nappend(clients[conn].cmd, buf + i, rbytes);
		i += rbytes;		
		if (clients[conn].cmd->len == tcp_serv_psize) {			
			/* process command */
			tcpserver_write(server, conn, clients[conn].cmd->val, clients[conn].cmd->len);			
			/* end process */
			str_set(clients[conn].cmd, "");
		}
	}
}

void server_disconnect(TcpServer *server, int conn, int gracefully)
{
	printf("Client %d left...\n", conn);
	clients[conn].active = 0;
}

void tcp_server_func(int psize)
{	
	for (int i = 0; i <64; i++)
		clients[i].cmd = new_str();
		
	tcp_serv_psize = psize;
		
	TcpServer *server = new_tcpserver();
	tcpserver_init(server, 1234);
	tcpserver_set_handlers(server, &server_read, &server_accept, &server_disconnect);
	tcpserver_listen(server);
	
	printf("Server listens...\n");
	
	while (1) {
		tcpserver_select(server);
	}
}

int pps = 0;

void client_read(TcpClient *client, char *buf, int len)
{
	int i = 0;	
	while (i < len) {
		int nbytes = clients[0].psize - clients[0].cmd->len;
		int abytes = len - i;
		int rbytes = abytes<nbytes ? abytes:nbytes;
		str_nappend(clients[0].cmd, buf + i, rbytes);
		i += rbytes;
		if (clients[0].cmd->len == clients[0].psize) {
			/* process command */
			tcpclient_write(client, clients[0].cmd->val, clients[0].cmd->len);			
			pps++;
			/* end process */
			str_set(clients[0].cmd, "");
		}
	}
}

void client_disconnect(TcpClient *client)
{
	printf("\n:(\n");
	exit(1);
}

void tcp_client_func(char *ip, int psize)
{
	clients[0].psize = psize;
	clients[0].cmd = new_str();
	
	TcpClient *socket = new_tcpclient();
	tcpclient_init(socket, 1234, ip);
	tcpclient_set_handlers(socket, &client_read, &client_disconnect);
	printf("Connecting...\n");
	tcpclient_connect(socket);
		
	tcpclient_write(socket, sockbuf, psize);
	
	float ppsTimer = glfwGetTime();
	
	while (1) 
	{
		tcpclient_select(socket);
		
		float currTime = glfwGetTime();
		if (currTime > ppsTimer + 1.0) {
			printf("PPS: %d\t\tAvg ping: %.0f ms\n", pps, (pps!=0 ? 1000.0f/(float)pps : 0));
			pps=0;
			ppsTimer = currTime;
		}
	}
}



//=======================================================================================


void udp_server_func(float retry)
{			
	SockAddrs *addrs = new_sockaddrs(64);
	caddr = new_sockaddr();
	UdpSocket *server = new_udpsocket(NULL, 1234);
	udpsocket_listen(server);
	
	printf("Server listens...\n");
				
	while (1) {				
		/* Try read something */
		memset(sockbuf, 0, 1501);
		int n = udpsocket_read(server, sockbuf, caddr);
		if (n > 0) {
			int clt = sockaddrs_add(addrs, caddr);
			/* process message */	
			int dataId;
			memcpy(&dataId, sockbuf, 4);
			
			if (!clients[clt].active) {
				clients[clt].active = 1;				
				clients[clt].psize = n;
				clients[clt].dataId = dataId;
				clients[clt].lastSendTime = 0;
				memcpy(clients[clt].currentData, sockbuf, n);
				printf("New client detected...\n");
			}
						
			/* check if data is newer advanced */
			if (dataId > clients[clt].dataId) {
				memcpy(clients[clt].currentData, sockbuf, clients[clt].psize);
				clients[clt].lastSendTime = 0;				
			}
		}
		
		float currTime = glfwGetTime();
		for (int i = 0; i < 64; i++) {
			if (clients[i].active) {	
				if (currTime > clients[i].lastSendTime + retry) {
					udpsocket_write(server, clients[i].currentData, clients[i].psize, sockaddrs_get(addrs, i));
					clients[i].lastSendTime = currTime;
				}
			}
		}					
	}
}

//***************************************************************************************

void udp_client_func(char *ip, int psize, float retry)
{
	UdpSocket *socket = new_udpsocket(ip, 1234);
	caddr = new_sockaddr();
			
	double lastSendTime = glfwGetTime();	
	double ppsTimer = glfwGetTime();	
	int numparts = 0;
	char previousData[1501];
	char currData[1501];
	int dataId = 0;
	memcpy(currData, &dataId, 4);	
		
	while (1) 
	{
		/* Try read something */
		memset(sockbuf, 0, psize);
		int n = udpsocket_read(socket, sockbuf, caddr);
		if (n > 0) {
			/* process reply */
			if (memcmp(sockbuf, currData, psize) == 0) {
				pps++;
				dataId++;
												
				/* data header */
				memcpy(currData, &dataId, 4); 				
				/* prepare new data */
				for (int i = 4; i < psize; i++) {
					currData[i] = 'A'+ (rand()%('Z'-'A')) + ('a'-'A') * (rand()%2);
				}
				/* force send new data immediately */
				lastSendTime = 0;				
			}
		}
		
		float currTime = glfwGetTime();
		if (currTime > lastSendTime + retry) {
			udpsocket_write(socket, currData, psize, udpsocket_get_addr(socket));
			lastSendTime = currTime;			
		}
		
		if (glfwGetTime() > ppsTimer + 1.0) {
			printf("PPS: %d\t\tAvg ping: %.0f ms\n", pps, (pps!=0 ? 1000.0f/(float)pps : 0));
			pps=0;
			ppsTimer = currTime;
		}
	}
}

int main(int argc, char **argv)
{
	socket_startup();
	glfwInit();
	
	if (argc > 2) {
		if (strcmp(argv[1], "-u")==0 || strcmp(argv[1],"--udp")==0) {
			if (strcmp(argv[2], "--server")==0 && argc==4) {
				float resend = atof(argv[3]);
				udp_server_func(resend);
			}
			else if (strcmp(argv[2], "--client")==0 && argc==6) {			
				int psize = atoi(argv[4]);				
				float resend = atof(argv[5]);
				udp_client_func(argv[3], psize,resend);
			}
		}		
		else
		if (strcmp(argv[1], "-t")==0 || strcmp(argv[1],"--tcp")==0) {			
			if (strcmp(argv[2], "--server")==0 && argc==4) {				
				int psize = atoi(argv[3]);						
				tcp_server_func(psize);
			}
			else if (strcmp(argv[2], "--client")==0 && argc==5) {			
				int psize = atoi(argv[4]);								
				tcp_client_func(argv[3], psize);
			}
		}		
	}	
	
	socket_cleanup();
	return 0;
}
