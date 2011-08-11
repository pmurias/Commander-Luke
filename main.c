#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "window.h"
#include "texture.h"
#include "socket.h"
#include "str.h"
#include "network.h"
#include "single_player.h"
#include "tcp_network.h"
#include "blit.h"
#include "font.h"
#include "iso.h"

#include "commands.h"
#include "camera.h"
#include "critter.h"

#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))

#define MAX_CLIENTS 20
Critter cri[MAX_CLIENTS];

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

void draw_tilemap(TileMap * map, Camera * cam, Sprite ** tileset)
{	
	iso_set_cam(cam->x, cam->y);
			
	for (int i = 0; i < cam->view * 2 + 1; i++)
		for (int j = 0; j < cam->view + 1 - (i % 2); j++) {
			float tileX = round(cam->x) + i / 2 - j;
			float tileY = round(cam->y) + j + i / 2 + (i % 2) - cam->view;

			if (tileX >= 0 && tileY >= 0 && tileX < map->width && tileY < map->height) {				
				Sprite *tile = tileset[map->tiles[(int)(map->width * tileY + tileX)]];
				float scrX, scrY;
				iso_world2screen(tileX, tileY, &scrX, &scrY);				
				
				blit_sprite(
					tile, 
					round(scrX - iso_tile_width()/2),
					round(scrY - iso_tile_height()/2)
				);
				
			}
		}
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

Sprite *g_tileset[3];
void load_assets()
{
	g_tileset[0] = blit_load_sprite("./data/tiles/template.png");
	g_tileset[1] = blit_load_sprite("./data/tiles/grass1.png");
	g_tileset[2] = blit_load_sprite("./data/tiles/walln.png");
	
	blit_load_spritesheet("./data/SheetNolty.png", "./data/SheetNolty.txt");
	
	font_load("./data/font/jura.png", "./data/font/jura.fnt");
	font_load("./data/font/ubuntu.png", "./data/font/ubuntu.fnt");
	
	printf("Building animations...");
	isoanim_build("Nolty.Idle", 10, 0.3);
	isoanim_build("Nolty.Running", 15, 0.03);
	printf("OK\n");
}

void command_set_tile(Engine * engine, Netcmd_SetTile * c)
{	
	engine->map->tiles[c->tile_y * engine->map->width + c->tile_x] = c->type;
}
void command_move_critter(Netcmd_MoveCritter *c)
{
	cri[c->sender].state = CRI_RUNNING;
		
	cri[c->sender].move_x = c->move_x;
	cri[c->sender].move_y = c->move_y;
}

void client_loop(NetworkType * network)
{
	window_open(800, 600, 0, "Commander Luke");	
	load_assets();	

	Engine *engine = engine_init();
	Camera *camera = camera_init();
		
	for (int i = 0; i < MAX_CLIENTS; i++) {
		cri[i].x = 0;
		cri[i].y = 0;
		cri[i].velocity = 0;
		cri[i].face_x = 0;
		cri[i].face_y = 1;
		cri[i].state = CRI_IDLE;
	}
	
	float time_step = 1.0/30.0;	
	float time_accum = 0;
	int running = 1;
	while (running) {
		window_start_frame();
		
		network->tick(network->state);		
		time_accum += window_frame_time();				
		while (time_accum >= time_step) {			
			time_accum -= time_step;			
			
			if (window_keypressed('X')) {
				running = 0;
				break;
			}			
			
			if (window_mousedown(0)) {				
				Netcmd_MoveCritter command;
				command.header.type = NETCMD_MOVECRITTER;	
				command.sender = network->get_id(network->state);
				iso_screen2world(camera->x - window_width() / 2 + window_xmouse(), camera->y - window_height() / 2 + window_ymouse(), &command.move_x, &command.move_y);
				network->add_command(network->state, (void *)&command);
			}

			Netcmd *command;
			while ((command = network->get_command(network->state))) {
				switch (command->header.type) {
				case NETCMD_SETTILE:{
						command_set_tile(engine, (Netcmd_SetTile *) command);					
						break;
					}
				case NETCMD_MOVECRITTER:{
						command_move_critter((Netcmd_MoveCritter *) command);
						break;
					}
				default:
					printf("Unknown command\n");
				}
				free(command);
			}
			
			for (int i = 0; i< MAX_CLIENTS; i++) {
				critter_tick(&cri[i]);
			}

			network->logic_tick(network->state);					
			camera->x = cri[network->get_id(network->state)].x;
			camera->y = cri[network->get_id(network->state)].y;
			window_poll_events();
		}
					
		draw_tilemap(engine->map, camera, g_tileset);			
		
		for (int i = 0; i< MAX_CLIENTS; i++) {
			critter_draw(&cri[i]);
		}		
		
		font_print(font_get("Jura"), 10, 10, 1.0, "Hello World!\nFPS: %d", (int)round(1.0/window_frame_time()	));		

		window_end_frame();				
	}

	network->cleanup(network->state);
	camera_free(camera);
	engine_free(engine);
	window_close();
}


void server_loop(NetworkType *network)
{		
	while (1) {
		network->tick(network->state);		
	}
}

void system_startup()
{
	socket_startup();
	commands_startup();
	blit_startup();
	font_startup();
	iso_startup(160, 80);
}

int main(int argc, char **argv)
{	
	system_startup();
		
	if (argc > 1) {
		if (strcmp(argv[1], "--server") == 0) {
			server_loop(new_tcp_server_state());
		} else if (strcmp(argv[1], "--client") == 0 && argc == 3) {
			client_loop(new_tcp_client_state(argv[2], "1234"));
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
