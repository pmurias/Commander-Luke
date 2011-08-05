#include <GL/glfw.h>
#include "camera.h"
#include <stdlib.h>
Camera *camera_init()
{
    Camera *camera = malloc(sizeof(Camera));
    camera->x = 0;
    camera->y = 0;
    camera->dx = 0;
    camera->dy = 0;
    camera->view = 9;
    return camera;
}

void camera_free(Camera * camera)
{
    free(camera);
}

void camera_keyboard_control(Camera * camera)
{
    if (glfwGetKey(GLFW_KEY_RIGHT))
	camera->dx += (3.0 - camera->dx) * 0.1;
    if (glfwGetKey(GLFW_KEY_LEFT))
	camera->dx += (-3.0 - camera->dx) * 0.1;
    if (glfwGetKey(GLFW_KEY_DOWN))
	camera->dy += (3.0 - camera->dy) * 0.1;
    if (glfwGetKey(GLFW_KEY_UP))
	camera->dy += (-3.0 - camera->dy) * 0.1;
    camera->x += camera->dx;
    camera->y += camera->dy;
    camera->dx *= 0.97;
    camera->dy *= 0.97;
}
