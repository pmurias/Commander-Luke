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
	int mx;
	int my;
	int ready_mx;
	int ready_my;
	int ready;
	int active;	
	int start_frame;
} CommonData;

CommonData s_clients[64];
char sockbuf[1024];
SockAddr *caddr = NULL;

void server_func()
{	
	glfwInit();	
	
	SockAddrs *clients = new_sockaddrs(64);
	UdpSocket *server = new_udpsocket("localhost", 1234);
	udpsocket_listen(server);
	
	printf("Server listens...\n");
	
	int currFrame = 0;
	double lastSendTime = glfwGetTime();
	
	while (1) {				
		/* Try read something */
		memset(sockbuf, 0, 1024);
		int n = udpsocket_read(server, sockbuf, &caddr);
		if (n > 0) {
			int clt = sockaddrs_add(clients, caddr);
			/* process message */
			int f, mx, my;
			sscanf(sockbuf, "%d %d %d", &f, &mx, &my);
			
			if (!s_clients[clt].active) {
				s_clients[clt].active = 1;
				s_clients[clt].start_frame = currFrame;
			}
			/* check if it isn't missed message */
			if (s_clients[clt].start_frame  + f == currFrame) {
				s_clients[clt].mx = mx;
				s_clients[clt].my = my;
				s_clients[clt].ready = 1;
			}
		}
				
		/* check if all clients reported current frame */
		int all_ready = 1;
		for (int i = 0; i<64; i++) {
			if (s_clients[i].active && !s_clients[i].ready) all_ready = 0;			
		}
		
		/* if so, we can swap current 'broadcast' data  */
		if (all_ready) {
			int actives = 0;
			for (int i=0; i<64; i++) {
				if (s_clients[i].active) {
					s_clients[i].ready_mx = s_clients[i].mx;
					s_clients[i].ready_my = s_clients[i].my;
					s_clients[i].ready = 0;
					actives++;
				}
			}			
			if (actives > 0) {
				currFrame++;
				lastSendTime -= 1.0; // force send
				printf("Waiting for frame %d\n", currFrame);
			}
		}
		
		
		if (glfwGetTime() > lastSendTime + 0.3) {
			for (int i = 0; i<64; i++) {				
				if (s_clients[i].active) {				
					for (int j=0; j<64; j++) {
						if (s_clients[j].active && !s_clients[j].ready) {
							int frameNoAtJ = currFrame-1-s_clients[j].start_frame;							
							memset(sockbuf, 0, 1024);						
							sprintf(sockbuf, "%d %d %d %d", i, frameNoAtJ, s_clients[i].ready_mx, s_clients[i].ready_my);
							udpsocket_write(server, sockbuf, strlen(sockbuf), sockaddrs_get(clients, j));						
						}
					}				
				}
			}				
			lastSendTime = glfwGetTime();
		}
	}
}

//***************************************************************************************

void client_func(char *ip)
{
	UdpSocket *socket = new_udpsocket(ip, 1234);
	
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
		
	int currFrame = 0;
	int ready_mx = 0;
	int ready_my = 0;
	
	while (1) 
	{
		/* Try read something */
		memset(sockbuf, 0, 1024);
		int n = udpsocket_read(socket, sockbuf, &caddr);
		if (n > 0) {
			/* !! should check if caddr is really our server !! */
			
			/* process message */
			int clt, f, mx, my;
			sscanf(sockbuf, "%d %d %d %d", &clt, &f, &mx, &my);
			
			if (s_clients[clt].active == 0)
				s_clients[clt].active = 1;
					
			if (f == currFrame) {				
				s_clients[clt].mx = mx;
				s_clients[clt].my = my;
				s_clients[clt].ready = 1;
			}
		}
		
		/* check if all co-clients reported current frame */
		int all_ready = 1;
		for (int i = 0; i<64; i++) {
			if (s_clients[i].active && !s_clients[i].ready) all_ready = 0;			
		}
		
		int mouseX, mouseY;
		glfwGetMousePos(&mouseX, &mouseY);
		
		if (all_ready) {
			int actives =0;
			for (int i = 0; i<64; i++) {
				if (s_clients[i].active) {
					s_clients[i].ready_mx = s_clients[i].mx;
					s_clients[i].ready_my = s_clients[i].my;
					s_clients[i].ready = 0;				
					actives++;
				}
			}
			if (actives > 0) {
				currFrame++;
				lastSendTime -= 1.0; // force send
				printf("Waiting for frame %d\n", currFrame);
			}
			ready_mx = mouseX;
			ready_my = mouseY;			
		}
						
		if (glfwGetTime() > lastSendTime + 0.2) {				
			memset(sockbuf, 0, 1024);
			sprintf(sockbuf, "%d %d %d\n", currFrame, ready_mx, ready_my);		
			udpsocket_write(socket, sockbuf, strlen(sockbuf), udpsocket_get_addr(socket));
			lastSendTime = glfwGetTime();			
		} else continue;
				
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
				blit(g_mouseTexture, s_clients[i].ready_mx, s_clients[i].ready_my);
			}			
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