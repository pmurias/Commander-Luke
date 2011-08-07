#include "network.h"
#include "socket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef struct Command {
    struct Command* next;
    int type;
    void* buf;
    int size;
} Command;
typedef struct {
  Command* last;
  Command* first;
  void* last_buf;
  TcpClient* client;
} Data;

static void tick(void* d) {
  Data* data = (Data*) d;
  tcpclient_select(data->client);
}
static void logic_tick(void* d) {
  Data* data = (Data*) d;
}
static void add_command(void* data,int type,int size,void *buf) {
  printf("add_command\n");
  Data* commands = (Data*) data;

  Command* c = (Command*) malloc(sizeof(Command)); 
  c->type = type;
  c->buf = malloc(size);
  memcpy(c->buf,buf,size);
  c->size = size;
  c->next = NULL;
  
  if (!commands->first) {
    commands->first = c;
    commands->last = c;
  } else {
    commands->last->next = c;
  }
}

void client_read(TcpClient *client, char *buf, int len)
{
}
void client_disconnect(TcpClient *client)
{
  printf("Disconnect\n");
}

static void* get_command(void* data,int *type,int *size) {
  /*Data* commands = (Data*) data;
  if (commands->last_buf) {
    free(commands->last_buf);
    commands->last_buf = NULL;
  }
  if (!commands->first) {
    return NULL; 
  } else {
    Command* ret = commands->first;
    commands->first = ret->next;
    if (commands->last == ret) {
      commands->last = NULL;
    }
    *type = ret->type;
    *size = ret->size;
    commands->last_buf = ret->buf;
    return ret->buf;
  }*/
  return NULL;
}
static void cleanup(void* data) {
  Data* commands = (Data*) data;
  if (commands->last_buf) free(commands->last_buf);

  Command* c = commands->first;
  while (c) {
    free(c->buf);
    c = c->next;
  }
  socket_cleanup();

}

NetworkType* tcp_network(char* ip,char* port) {

  socket_startup();

  printf("here\n");
  NetworkType* tcp = malloc(sizeof(NetworkType));
  tcp->tick = tick;
  tcp->logic_tick = logic_tick;
  tcp->add_command = add_command;
  tcp->get_command = get_command;
  tcp->cleanup = cleanup;

  Data* data = (Data*)malloc(sizeof(Data));
  tcp->data = (void*)data;

  data->last = NULL;
  data->first = NULL;
  data->last_buf = NULL;

  printf("creating\n");

  data->client = new_tcpclient();
  tcpclient_init(data->client, 1234, ip);

  /*tcpclient_set_handlers(socket, &client_read, &client_disconnect);*/
  printf("Connecting...\n");
  tcpclient_set_handlers(data->client, &client_read, &client_disconnect);
  tcpclient_connect(data->client);
  printf("there\n");

  return tcp;
}
