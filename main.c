#include <GL/glfw.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "texture.h"
#include "socket.h"
#include "str.h"
#include "camera.h"
#include "network.h"
#include "single_player.h"
#include "commands.h"
#include "tcp_network.h"

int g_screenWidth = 800;
int g_screenHeight = 600;
int g_tileWidth = 160;
int g_tileHeigth = 80;

#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))

typedef struct {
	int width;
	int height;
	int *tiles;
} TileMap;

TileMap *tilemap_init(int w, int h)
{
	TileMap *map = malloc(sizeof(TileMap));
	map->tiles = NEWC(int, w * h);
	map->width = w;
	map->height = h;
	memset(map->tiles, 0, w * h * 4);
	return map;
}

void tilemap_free(TileMap * map)
{
	free(map->tiles);
	free(map);
}

void blit(Texture * tex, int x, int y)
{
	if (x + tex->width >= 0 && x <= g_screenWidth && y + tex->height >= 0 && y <= g_screenHeight) {
		glBindTexture(GL_TEXTURE_2D, tex->handle);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(x, y);
		glTexCoord2f(0, 1);
		glVertex2f(x, y + tex->height);
		glTexCoord2f(1, 1);
		glVertex2f(x + tex->width, y + tex->height);
		glTexCoord2f(1, 0);
		glVertex2f(x + tex->width, y);
		glEnd();
	}
}

void tile_to_screen(float x, float y, float *ox, float *oy)
{
	*ox = (y - x) * 80.0;
	*oy = (x + y) * 40.0;
}

void snap_screen_to_tile(float x, float y, float *ox, float *oy)
{
	*ox = round(y / 80.0f - x / 160.0f);
	*oy = round(x / 160.0f + y / 80.0f);
}

void draw_tilemap(TileMap * map, Camera * cam, Texture ** tileset)
{
	float tCamX, tCamY;
	float centerX, centerY;
	float offX, offY;
	snap_screen_to_tile(cam->x, cam->y, &tCamX, &tCamY);
	tile_to_screen(tCamX, tCamY, &centerX, &centerY);
	offX = centerX - cam->x;
	offY = centerY - cam->y;
	for (int i = 0; i < cam->view * 2 + 1; i++)
		for (int j = 0; j < cam->view + 1 - (i % 2); j++) {
			float tileX = tCamX + i / 2 - j;
			float tileY = tCamY + j + i / 2 + (i % 2) - cam->view;

			if (tileX > 0 && tileY > 0 && tileX < map->width && tileY < map->height) {
				Texture *tile = tileset[map->tiles[(int)(map->width * tileY + tileX)]];
				float scrX, scrY;
				tile_to_screen(tileX - tCamX, tileY - tCamY, &scrX, &scrY);
				blit(tile, round(offX + scrX + g_screenWidth / 2 - 80), round(offY + scrY + g_screenHeight / 2 - (tile->height - 40)));
			}
		}
}

void opengl_start()
{
	glfwInit();
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 1);
	glfwOpenWindow(g_screenWidth, g_screenHeight, 8, 8, 8, 0, 0, 0, GLFW_WINDOW);
	glfwSetWindowTitle("Commander Luke");
	//glfwDisable(GLFW_MOUSE_CURSOR);

	glOrtho(0, g_screenWidth, g_screenHeight, 0, -1, 1);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void opengl_stop()
{
	glfwTerminate();
}

typedef struct {
	TileMap *map;
} Engine;

void usage()
{
	printf("Usage: luke [--server] [--client ip]\n");
}

Engine *engine_init()
{
	Engine *engine = malloc(sizeof(Engine));
	engine->map = tilemap_init(100, 100);
	return engine;
}

void engine_free(Engine * engine)
{
	tilemap_free(engine->map);
	free(engine);
}

Texture *g_tileset[3];
void load_assets()
{
	g_tileset[0] = texture_from_file("./data/tiles/template.png");
	g_tileset[1] = texture_from_file("./data/tiles/grass1.png");
	g_tileset[2] = texture_from_file("./data/tiles/walln.png");
}

void command_set_tile(Engine * engine, Netcmd_SetTile * c)
{	
	engine->map->tiles[c->tile_y * engine->map->width + c->tile_x] = c->type;
}

void client_loop(NetworkType * network)
{
	opengl_start();
	load_assets();

	Engine *engine = engine_init();
	Camera *camera = camera_init();

	while (1) {
		network->tick(network->state);

		int mouseX, mouseY;
		glfwGetMousePos(&mouseX, &mouseY);

		if (glfwGetKey('X'))
			break;

		if (glfwGetMouseButton(0)) {
			float tileX, tileY;
			snap_screen_to_tile(camera->x - g_screenWidth / 2 + mouseX, camera->y - g_screenHeight / 2 + mouseY, &tileX, &tileY);
			if (tileX >= 0 && tileY >= 0) {
				Netcmd_SetTile command;
				command.header.type = NETCMD_SETTILE;
				command.tile_x = tileX;
				command.tile_y = tileY;
				command.type = 1;
				network->add_command(network->state, (void *)&command);
			}
		}

		
		Netcmd *command;
		while ((command = network->get_command(network->state))) {
			switch (command->header.type) {
			case NETCMD_SETTILE:{
					command_set_tile(engine, (Netcmd_SetTile *) command);					
					break;
				}
			default:
				printf("Unknown command\n");
			}
			free(command);
		}

		network->logic_tick(network->state);

		camera_keyboard_control(camera);

		glClear(GL_COLOR_BUFFER_BIT);

		draw_tilemap(engine->map, camera, g_tileset);

		glfwSwapBuffers();
	}

	network->cleanup(network->state);
	camera_free(camera);
	engine_free(engine);
	opengl_stop();
}

#define MAX_CLIENTS 20

typedef struct {
	char *read_buf;
	uint32_t num_read_bytes;
	uint32_t msg_size;
	int waiting;
	int needs_greeting;
	int active;
} Client;

char bulk_packet[8196];
uint32_t bulk_packet_size = 4;

Client clients[MAX_CLIENTS];

int server_accept(TcpServer * server, int conn)
{
	clients[conn].active = 1;
	clients[conn].msg_size = 0;
	clients[conn].waiting = 0;
	clients[conn].needs_greeting = 1;
	printf("Client %d connected...\n", conn);
	return 1;
}

void server_disconnect(TcpServer * server, int conn, int gracefully)
{
	clients[conn].active = 0;
	printf("Client %d left gracefully:%d...\n", conn, gracefully);
}

void server_read(TcpServer * server, int conn, char *buf, int len)
{
	int i = 0;
	while (i != len) {
		if (clients[conn].msg_size == 0) {
			clients[conn].msg_size = *(uint32_t*)(buf + i);			
			clients[conn].read_buf = malloc(clients[conn].msg_size);						
			clients[conn].num_read_bytes = 0;
			i += sizeof(uint32_t);
		}
		
		int nbytes = clients[conn].msg_size - clients[conn].num_read_bytes;						
		int rbytes = (len - i) < nbytes ? (len - i) : nbytes;
		memcpy(clients[conn].read_buf + clients[conn].num_read_bytes, buf + i, rbytes);
		i += rbytes;
		clients[conn].num_read_bytes += rbytes;				
		
		if (clients[conn].num_read_bytes == clients[conn].msg_size) {
			memcpy(bulk_packet + bulk_packet_size, clients[conn].read_buf, clients[conn].msg_size );
			bulk_packet_size +=  clients[conn].msg_size;
			free(clients[conn].read_buf);
			clients[conn].waiting = 0;
			clients[conn].msg_size = 0;
		}
	}
}

void server_loop()
{
	Engine *engine = engine_init();

	TcpServer *server = new_tcpserver();

	tcpserver_init(server, 1234);
	tcpserver_set_handlers(server, &server_read, &server_accept, &server_disconnect);
	tcpserver_listen(server);

	printf("Server listens...\n");

	while (1) {
		tcpserver_select(server);
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (clients[i].needs_greeting) {			
				char client_id = i;				
				tcpserver_write(server, i, &client_id, 1);
				clients[i].needs_greeting = 0;
				clients[i].waiting = 1;
			}
		}
		
		int none_waits = 1;
		for (int i = 0; i< MAX_CLIENTS; i++) {
			none_waits &= !(clients[i].active && clients[i].waiting);
		}
				
		if (none_waits) {
			memcpy(bulk_packet, &bulk_packet_size, sizeof(uint32_t));			
			for (int i = 0; i< MAX_CLIENTS; i++) {				
				if (clients[i].active) {				
					tcpserver_write(server, i, bulk_packet, bulk_packet_size);
					clients[i].waiting = 1;
				}
			}
			bulk_packet_size = 4;
		}
	}
}

void system_startup()
{
	socket_startup();
	command_startup();
}

int main(int argc, char **argv)
{	
	system_startup();
	
	if (argc > 1) {
		if (strcmp(argv[1], "--server") == 0) {
			server_loop();
		} else if (strcmp(argv[1], "--client") == 0 && argc == 3) {
			client_loop(tcp_network(argv[2], "1234"));
		} else {
			usage();
		}
	} else if (argc == 1) {
		client_loop(single_player_network());
	} else {
		usage();
	}
	
	socket_startup();

	return 0;
}
