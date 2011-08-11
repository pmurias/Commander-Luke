#ifndef __CRITTER_H__
#define __CRITTER_H__

#define CRI_IDLE 0
#define CRI_RUNNING 1

typedef struct
{	
	float x;
	float y;
	
	float velocity;
	
	float face_x;
	float face_y;
	
	float anim_time;
	
	int state;
	
	float move_x;
	float move_y;
} Critter;

void critter_tick(Critter *cri);
void critter_draw(Critter *cri);

#endif // __CRITTER_H__