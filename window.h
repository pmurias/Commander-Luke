#ifndef __WINDOW_H__
#define __WINDOW_H__

void window_open(int w, int h, int fullscreen, char *title);
void window_close(void);
void window_poll_events(void);
void window_start_frame(void);
void window_end_frame(void);
int window_keypressed(int key);
int window_keydown(int key);
int window_xmouse(void);
int window_ymouse(void);
int window_mousepressed(int button);
int window_mousedown(int button);
int window_width(void);
int window_height(void);
double window_frame_time(void);

#endif
