#include <stdio.h>

#include "ai.h"
#include "critter.h"
#include "rand.h"

uint32_t ai_seed = 1;

const AiFunc ai_funcs[] = {
	ai_noop,
	ai_run_around
};

void ai_noop(Critter *c) {
}
void ai_run_around(Critter *c)
{	
  	float velocity = c->vtable->get_velocity(c);
	if (velocity == 0) {		
		Netcmd_MoveCritter command;
		command.header.type = NETCMD_MOVECRITTER;
		command.move_x = 50+(rand_rand_r(&ai_seed)%400)/100.0;
		command.move_y = 50+(rand_rand_r(&ai_seed)%400)/100.0;
		c->vtable->order(c,(Netcmd*) &command);
		printf("ai_seed: %u\n", ai_seed);
	}
}
