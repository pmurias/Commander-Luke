#ifndef __AI_H__
#define __AI_H__

struct Critter;
typedef struct Critter Critter;

typedef void (*AiFunc)(Critter *c);

#define AI_NOOP 			0
void ai_noop(Critter* c);

#define AI_RUN_AROUND 	1
void ai_run_around(Critter* c);

extern const AiFunc ai_funcs[];
extern uint32_t ai_seed;

#endif
