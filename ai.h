#ifndef __AI_H__
#define __AI_H__

struct Critter;
typedef struct Critter Critter;

typedef void (*AiFunc)(Critter *c);

void ai_noop(Critter* c);
void ai_run_around(Critter* c);

#endif
