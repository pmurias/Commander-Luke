#ifndef __TCP_NETWORK_H__
#define __TCP_NETWORK_H__

#define TCP_MSG_NONE 0
#define TCP_MSG_CMDS 1
#define TCP_MSG_LOGIN 2
#define TCP_MSG_SNAPSHOT 3
#define TCP_MSG_CONFIRM 4
#define TCP_MSG_CID 5

typedef void (*SnapshotCallback)(void **buf, uint8_t cid, uint32_t *size);
typedef int (*LoginCallback)(void *buf, uint8_t cid, uint32_t size);

typedef void (*ClientSnapshotCallback)(void *buf, uint32_t size);
typedef void (*NewTurnCallback)(void);

NetworkType* new_tcp_client_state(char* ip, int port, float *ticks);
void tcpclientstate_set_snapshot_callback(void *d, ClientSnapshotCallback cb);
void tcpclientstate_set_newturn_callback(void *d, NewTurnCallback cb);
void tcpclientstate_wait_for_snapshot(void *d);
void tcpclientstate_login(void *d, void *login_data, uint32_t ldsize);
NetworkType *new_tcp_server_state(float *ticks);
void tcpserverstate_set_snapshot_callback(void *state, SnapshotCallback cb);
void tcpserverstate_set_login_callback(void *state, LoginCallback cb);
void tcpserverstate_set_turnsent_callback(void *state, NewTurnCallback cb);

#endif
