#include <GL/glfw.h>

#include "window.h"

typedef struct
{
	int width;
	int height;
	int fullscreen;
	
	int keypressed[GLFW_KEY_LAST+1];
	int keydown[GLFW_KEY_LAST+1];
	
	int mousepressed[GLFW_MOUSE_BUTTON_LAST+1];
	int mousedown[GLFW_MOUSE_BUTTON_LAST+1];
	int xmouse;
	int ymouse;	
	
	double frame_time;
	double last_frame_time;
} WindowState;
static WindowState ws;

//-----------------------------------------------------------------------------
void window_open(int w, int h, int fullscreen, char *title)
{
	ws.width = w;
	ws.height = h;
	ws.fullscreen = fullscreen;
	
	glfwInit();
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 1);
	glfwOpenWindow(w, h, 8, 8, 8, 0, 0, 0, fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW);
	glfwSetWindowTitle(title);
	glfwDisable(GLFW_AUTO_POLL_EVENTS);
	
	glOrtho(0, w, h, 0, -1, 1);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
	ws.last_frame_time = glfwGetTime();
}

//-----------------------------------------------------------------------------
void window_start_frame(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
}

//-----------------------------------------------------------------------------
void window_poll_events(void)
{
	
	for (int i = 0; i < GLFW_KEY_LAST+1; i++) {
		ws.keypressed[i] = 0;
		if (glfwGetKey(i)) {
			if (!ws.keydown[i]) {
				ws.keypressed[i] = 1;
			}
			ws.keydown[i] = 1;
		} else {
			ws.keydown[i] = 0;
		}
	}
	for (int i = 0; i< GLFW_MOUSE_BUTTON_LAST+1; i++) {
		ws.mousepressed[i] = 0;
		if (glfwGetMouseButton(i)) {
			if (!ws.mousedown[i]) {
				ws.mousepressed[i] = 1;
			}
			ws.mousedown[i] = 1;
		} else {
			ws.mousedown[i] = 0;
		}		
	}
	
	glfwPollEvents();
}

//-----------------------------------------------------------------------------
void window_end_frame(void)
{
	ws.frame_time = glfwGetTime() - ws.last_frame_time;
	ws.last_frame_time += ws.frame_time;
		
	glfwGetMousePos(&ws.xmouse, &ws.ymouse);
	glfwSwapBuffers();
}

//-----------------------------------------------------------------------------
int window_keypressed(int key)
{
	return ws.keypressed[key];
}
//-----------------------------------------------------------------------------
int window_keydown(int key)
{
	return ws.keydown[key];
}
//-----------------------------------------------------------------------------
int window_xmouse(void)
{
	return ws.xmouse;
}
//-----------------------------------------------------------------------------
int window_ymouse(void)
{
	return ws.ymouse;
}
//-----------------------------------------------------------------------------
int window_mousepressed(int button)
{
	return ws.mousepressed[button];
}
//-----------------------------------------------------------------------------
int window_mousedown(int button)
{
	return ws.mousedown[button];
}

//-----------------------------------------------------------------------------
void window_close(void)
{	
	glfwTerminate();
}

//-----------------------------------------------------------------------------
int window_width(void)
{
	return ws.width;
}

//-----------------------------------------------------------------------------
int window_height(void)
{
	return ws.height;
}

//-----------------------------------------------------------------------------
double window_frame_time(void)
{
	return ws.frame_time;
}
