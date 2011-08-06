#include "network.h"
#include <stdlib.h>
#include <string.h>
typedef struct SingleCommand {
    struct SingleCommand* next;
    int type;
    void* buf;
    int size;
} SingleCommand;
typedef struct {
  SingleCommand* last;
  SingleCommand* first;
  void* last_buf;
} SingleData;

static void noop(void* data) {
}
static void add_command(void* data,int type,int size,void *buf) {
  SingleData* commands = (SingleData*) data;

  SingleCommand* c = (SingleCommand*) malloc(sizeof(SingleCommand)); 
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

static void* get_command(void* data,int *type,int *size) {
  SingleData* commands = (SingleData*) data;
  if (commands->last_buf) {
    free(commands->last_buf);
    commands->last_buf = NULL;
  }
  if (!commands->first) {
    return NULL; 
  } else {
    SingleCommand* ret = commands->first;
    commands->first = ret->next;
    if (commands->last == ret) {
      commands->last = NULL;
    }
    *type = ret->type;
    *size = ret->size;
    commands->last_buf = ret->buf;
    return ret->buf;
  }
}

NetworkType* single_player_network(void) {
  NetworkType* single = malloc(sizeof(NetworkType));
  single->tick = noop;
  single->logic_tick = noop;
  single->add_command = add_command;
  single->get_command = get_command;

  SingleData* data = (SingleData*)malloc(sizeof(SingleData));
  single->data = (void*)data;
  data->last = NULL;
  data->first = NULL;
  data->last_buf = NULL;
  return single;
}
