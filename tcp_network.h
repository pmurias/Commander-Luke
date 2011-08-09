#ifndef __TCP_NETWORK_H__
#define __TCP_NETWORK_H__

NetworkType* new_tcp_client_state(char* ip,char* port);
NetworkType* new_tcp_server_state(void);

#endif
