#include "network.h"
#include <stdlib.h>
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
} Data;

static void noop(void* data) {
}
static void add_command(void* data,int type,int size,void *buf) {
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

static void* get_command(void* data,int *type,int *size) {
  Data* commands = (Data*) data;
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
  }
}
static void cleanup(void* data) {
  Data* commands = (Data*) data;
  if (commands->last_buf) free(commands->last_buf);

  Command* c = commands->first;
  while (c) {
    free(c->buf);
    c = c->next;
  }

}

NetworkType* single_player_network(void) {
  NetworkType* single = malloc(sizeof(NetworkType));
  single->tick = noop;
  single->logic_tick = noop;
  single->add_command = add_command;
  single->get_command = get_command;
  single->cleanup = cleanup;

  Data* data = (Data*)malloc(sizeof(Data));
  single->data = (void*)data;
  data->last = NULL;
  data->first = NULL;
  data->last_buf = NULL;
  return single;
}
