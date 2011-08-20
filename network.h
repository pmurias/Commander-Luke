#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

typedef struct {
	uint8_t type;	
} NetcmdHeader;

typedef struct {
	NetcmdHeader header;
	char data[1];
} Netcmd;

#pragma pack(pop)

int netcmd_sizes[255];

#define command_size(cmd) netcmd_sizes[(cmd)->header.type]

typedef struct {
    void* state;
    void (*add_command)(void* state, Netcmd *command);
    Netcmd* (*get_command)(void* state);
    void (*logic_tick)(void* state);
    void (*tick)(void* state);
	 uint8_t (*get_id)(void* state);
    void (*cleanup)(void* state);
} NetworkType;

#endif
