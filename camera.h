#ifndef __CAMERA_H__
#define __CAMERA_H__

typedef struct
{
	float x;
	float y;
	float dx;
	float dy;
	int view;
} Camera;

Camera *camera_init(void);
void camera_free(Camera * camera);
void camera_keyboard_control(Camera * camera);

#endif
