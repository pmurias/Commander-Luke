#ifndef __NETWORK_H__
#define __NETWORK_H__
typedef struct {
    void* data;
    void (*add_command)(void* data,int type,int size,void *command);
    /* the buffer returned by get_command is only guaranteed to be avalible till the next call to get_command*/
    void* (*get_command)(void* data,int *type,int *size);
    void (*logic_tick)(void* data);
    void (*tick)(void* data);
    void (*cleanup)(void* data);
} NetworkType;
#endif
