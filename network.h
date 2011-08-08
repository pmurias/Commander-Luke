#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "commands.h"

typedef struct {
    void* state;
    void (*add_command)(void* state, Netcmd *command);
    /* the buffer returned by get_command is only guaranteed to be avalible till the next call to get_command*/
    Netcmd* (*get_command)(void* state);
    void (*logic_tick)(void* state);
    void (*tick)(void* state);
    void (*cleanup)(void* state);
} NetworkType;

#endif
