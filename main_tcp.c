#include <GL/glfw.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "texture.h"
#include "socket.h"
#include "str.h"

#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))

/* Globalz */
int g_screenWidth = 800;
int g_screenHeight = 600;
int g_tileWidth = 160;
int g_tileHeigth = 80;

Texture *g_tileset[3];
Texture *g_mouseTexture;

Texture *texture_from_file(char *fname)
{
	Image img;
	Texture *tex = malloc(sizeof(Texture));
	
	image_load_from_file(&img, fname);
	texture_from_image(tex, &img);
	image_free(&img);
	return tex;
}

void blit(Texture *tex, int x, int y)
{	
	if (x + tex->width >= 0 && x <= g_screenWidth && y + tex->height >= 0 && y <= g_screenHeight) {							
		glBindTexture(GL_TEXTURE_2D, tex->handle);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(x, y);
		glTexCoord2f(0, 1);
		glVertex2f(x, y+tex->height);
		glTexCoord2f(1, 1);
		glVertex2f(x+tex->width, y+tex->height);
		glTexCoord2f(1, 0);
		glVertex2f(x+tex->width, y);
		glEnd();			
	}
}

typedef struct
{
	float x;
	float y;
	float dx;
	float dy;
	int view;
} Camera;

typedef struct
{
	int width;
	int height;
	int *tiles;
} TileMap;

void init_tilemap(TileMap *map, int w, int h)
{
	map->tiles = NEWC(int, w*h);
	map->width = w;
	map->height = h;
	memset(map->tiles, 0, w*h*4);
}
   
void tile_to_screen(float x, float y, float *ox, float *oy)
{
	*ox = (y-x)*80.0;
	*oy = (x+y)*40.0;
}

void snap_screen_to_tile(float x, float y, float *ox, float *oy)
{
	*ox = round(y/80.0f - x/160.0f);
	*oy = round(x/160.0f + y/80.0f);
}
						
void draw_tilemap(TileMap *map, Camera *cam, Texture **tileset)
{
	float tCamX, tCamY;
	float centerX, centerY;
	float offX, offY;
	snap_screen_to_tile(cam->x, cam->y, &tCamX, &tCamY);
	tile_to_screen(tCamX, tCamY, &centerX, &centerY);
	offX = centerX - cam->x;
	offY = centerY - cam->y;	
	for (int i = 0; i< cam->view*2+1; i++)
		for (int j = 0; j < cam->view+1-(i%2); j++)
		{
			float tileX = tCamX + i/2 - j;
			float tileY = tCamY + j + i/2 + (i%2) - cam->view;
			
			if (tileX > 0 && tileY > 0 && tileX < map->width && tileY < map->height)
			{				
				Texture *tile = tileset[map->tiles[(int)(map->width * tileY + tileX)]];
				float scrX, scrY;
				tile_to_screen(tileX - tCamX, tileY - tCamY, &scrX, &scrY);
				blit(tile, round(offX + scrX + g_screenWidth/2 - 80), round(offY + scrY+g_screenHeight/2 - (tile->height - 40)));
			}
		}
}

void start_opengl()
{
	glfwInit();	
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 1);
	glfwOpenWindow(g_screenWidth, g_screenHeight, 8, 8, 8, 0, 0, 0, GLFW_WINDOW);
	glfwSetWindowTitle("Commander Luke");	
	glfwDisable( GLFW_MOUSE_CURSOR );
		
	glOrtho(0, g_screenWidth, g_screenHeight, 0, -1, 1);	
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
}

typedef struct
{
	int mouse_x;
	int mouse_y;
	int ready;
	int active;
	Str *cmd;
} Server_ClientData;

Server_ClientData s_clients[64];
int s_actives = 0;

int server_accept(ServerSocket *server, int conn)
{
	printf("Client %d connected...\b", conn);	
	s_clients[conn].active = 1;
	s_clients[conn].ready = 0;
	s_actives++;
	return 1;
}

void server_read(ServerSocket *server, int conn, char *buf, int len)
{
	for (int i=0; i < len; i++) {
		if (buf[i] == '\n') {
			/* process command */
			sscanf(s_clients[conn].cmd->val, "%d %d", &s_clients[conn].mouse_x, &s_clients[conn].mouse_y);
			s_clients[conn].ready = 1;
			printf("Received (%d, %d) from %d\n", s_clients[conn].mouse_x, s_clients[conn].mouse_y, conn);
			/* end process */
			str_set(s_clients[conn].cmd, "");
		}
		else {
			str_nappend(s_clients[conn].cmd, buf + i, 1);
		}
	}	
}

void server_disconnect(ServerSocket *server, int conn, int gracefully)
{
	printf("Client %d left...\n", conn);
	s_clients[conn].active = 0;
	s_actives--;
}

void server_func()
{	
	for (int i=0; i<64; i++) {
		s_clients[i].cmd = new_str();
	}
	
	ServerSocket *server = new_serversocket();
	serversocket_init(server, 1234);
	serversocket_set_handlers(server, &server_read, &server_accept, &server_disconnect);
	serversocket_listen(server);
	
	printf("Server listens...\n");
	
	while (1) {
		serversocket_select(server);
		
		int all_ready = 1;
		for (int i = 0; i<64; i++) {
			if (s_clients[i].active && !s_clients[i].ready) all_ready = 0;			
		}
		
		if (all_ready && s_actives > 0) {				
		//	printf("All clients reported. Replying...\n");
			char buffer[255];
			for (int i = 0; i<64; i++) {				
				if (s_clients[i].active) {
					memset(buffer, 0, 255);
					sprintf(buffer, "%d %d %d\n", i, s_clients[i].mouse_x, s_clients[i].mouse_y);
					for (int j=0; j<64; j++) {
						if (s_clients[j].active) {							
							serversocket_write(server, j, buffer, strlen(buffer));							
						}
					}
					s_clients[i].ready = 0;
				}
			}
		}
		
	}
}

//***************************************************************************************
Str *c_buf;

void client_read(ClientSocket *client, char *buf, int len)
{
	for (int i=0; i < len; i++) {
		if (buf[i] == '\n') {
			/* process command */
			int con, mx, my;
			sscanf(c_buf->val, "%d %d %d", &con, &mx, &my);
			s_clients[con].mouse_x = mx;
			s_clients[con].mouse_y = my;
			s_clients[con].active = 1;
			s_clients[con].ready = 1;
			printf("Received %d -> (%d, %d)\n", con, s_clients[con].mouse_x, s_clients[con].mouse_y);
			/* end process */
			str_set(c_buf, "");
		}
		else {
			str_nappend(c_buf, buf + i, 1);
		}
	}	
}

void client_disconnect(ClientSocket *client)
{
	exit(1);
}

void client_func(char *ip)
{
	c_buf = new_str();
	
	ClientSocket *socket = new_clientsocket();
	clientsocket_init(socket, 1234, ip);
	clientsocket_set_handlers(socket, &client_read, &client_disconnect);
	printf("Connecting...\n");
	clientsocket_connect(socket);
	
	start_opengl();
			
	Camera camera;
	TileMap map;
	
	camera.x = 0;
	camera.y = 0;
	camera.dx = 0;
	camera.dy = 0;
	camera.view = 9;
		
	init_tilemap(&map, 100, 100);	
		
	g_tileset[0] = texture_from_file("./data/tiles/template.png");
	g_tileset[1] = texture_from_file("./data/tiles/grass1.png");
	g_tileset[2] = texture_from_file("./data/tiles/walln.png");
	g_mouseTexture = texture_from_file("./data/mouse.png");
	
	double lastSendTime = glfwGetTime();
	double fpsTimer = glfwGetTime();
	int frames = 0;
		
	while (1) 
	{
		int mouseX, mouseY;
		glfwGetMousePos(&mouseX, &mouseY);
		if (glfwGetTime() > lastSendTime + 0.05) {
			while (1) {					
				clientsocket_select(socket);
				
				int all_ready = 1;
				for (int i=0; i<64; i++) {
					if (s_clients[i].active && !s_clients[i].ready) all_ready = 0;
				}
				
				if (all_ready)
					break;
							
				if (glfwGetTime() > lastSendTime + 1.0) {
					for (int i=0; i<64; i++) {
						if (s_clients[i].active && !s_clients[i].ready) s_clients[i].active = 0;
					}
					break;
				}
			}
		
			char buf[255];
			memset(buf, 0, 255);
			sprintf(buf, "%d %d\n", mouseX, mouseY);		
			clientsocket_write(socket, buf, strlen(buf));
			lastSendTime = glfwGetTime();
			frames++;
		}
		if (glfwGetTime() > fpsTimer + 1.0) {
			printf("FPS: %d\n", frames);
			frames = 0;
			fpsTimer = glfwGetTime();
		}
		
		if (glfwGetKey('X'))
			break;	
		
		if (glfwGetKey(GLFW_KEY_RIGHT))
			camera.dx += (3.0 - camera.dx) * 0.1;
		if (glfwGetKey(GLFW_KEY_LEFT))
			camera.dx += (-3.0 - camera.dx) * 0.1;
		if (glfwGetKey(GLFW_KEY_DOWN))
			camera.dy += (3.0 - camera.dy) * 0.1;
		if (glfwGetKey(GLFW_KEY_UP))
			camera.dy += (-3.0 - camera.dy) * 0.1;
		camera.x += camera.dx;
		camera.y += camera.dy;
		camera.dx *= 0.97;
		camera.dy *= 0.97;		
		
		if (glfwGetMouseButton(0))
		{			
			float tileX, tileY;			
			snap_screen_to_tile(camera.x - g_screenWidth/2 + mouseX, camera.y - g_screenHeight/2 + mouseY, &tileX, &tileY);
			// map.tiles[(int)(tileY * map.width + tileX)] = 1;
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
						
		draw_tilemap(&map, &camera, g_tileset);
		
		for (int i=0; i<64; i++) {
			if (s_clients[i].active) {
				blit(g_mouseTexture, s_clients[i].mouse_x, s_clients[i].mouse_y);
			}
			s_clients[i].ready = 0;
		}
						
		glfwSwapBuffers();		
	}

	glfwTerminate();
}

int main(int argc, char **argv)
{
	socket_startup();
	
	if (argc > 1) {
		if (strcmp(argv[1], "--server")==0) {
			server_func();
		}
		else if (strcmp(argv[1], "--client")==0 && argc==3) {			
			client_func(argv[2]);
		}		
	}
	
	socket_cleanup();
	return 0;
}