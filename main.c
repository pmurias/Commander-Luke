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
#include "rand.h"
#include "array.h"

#include "commands.h"
#include "camera.h"
#include "critter.h"
#include "critters/human.h"
#include "critters/blurred.h"
#include "spell.h"
#include "spells/flare.h"

#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))

#define MAX_CLIENTS 20
Critter *cri[MAX_CLIENTS];
IsoLight *lights[MAX_CLIENTS];
int active[MAX_CLIENTS];
Array *spells;
Str *logins[MAX_CLIENTS];

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
	for (int i = 0; i < w * h; i++) map->tiles[i] = 1+rand_rand()%4;
	//memset(map->tiles, 0, w * h * 4);
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
				iso_blit_tile(tile->texture, tileX, tileY);				
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
	human_init_vtable();
	blurred_init_vtable();
	flare_init_vtable();
	Engine *engine = malloc(sizeof(Engine));
	engine->map = tilemap_init(100, 100);
	return engine;
}

void engine_free(Engine * engine)
{
	tilemap_free(engine->map);
	free(engine);
}

Sprite *g_tileset[12];
void load_assets()
{
	g_tileset[0] = blit_load_sprite("./data/tiles/template.png");
	g_tileset[1] = blit_load_sprite("./data/tiles/grass0.png");
	g_tileset[2] = blit_load_sprite("./data/tiles/grass1.png");
	g_tileset[3] = blit_load_sprite("./data/tiles/grass2.png");
	g_tileset[4] = blit_load_sprite("./data/tiles/grass3.png");		

	blit_load_spritesheet_split("./data/SheetNolty.png", "./data/SheetNolty.txt");
	blit_load_spritesheet("./data/blurred.png", "./data/blurred.txt");
	
	blit_load_sprite("./data/flare.png")->blend_mode = BLEND_ADD;	

	font_load("./data/font/jura.png", "./data/font/jura.fnt");
	font_load("./data/font/ubuntu.png", "./data/font/ubuntu.fnt");

	printf("Building animations...");
	isoanim_build("Nolty.Idle", 10, 0.3);
	isoanim_set_center(isoanim_get("Nolty.Idle"), 64, 20);
	isoanim_build("Nolty.Running", 15, 0.03);
	isoanim_set_center(isoanim_get("Nolty.Running"), 64, 20);
	printf("OK\n");
}

void command_set_tile(Engine * engine, Netcmd_SetTile * c)
{
	engine->map->tiles[c->tile_y * engine->map->width + c->tile_x] = c->type;
}

#include <GL/gl.h>
void client_loop(NetworkType * network)
{
	window_open(800, 600, 0, "Commander Luke");
	load_assets();

	Engine *engine = engine_init();
	Camera *camera = camera_init();

	for (int i = 0; i < MAX_CLIENTS; i++) {
		cri[i] = new_human(50, 50);
		lights[i] = new_isolight();
		lights[i]->r = 1;
		lights[i]->g = 0.5;
		lights[i]->b = 0.1;
		lights[i]->range = 0;
		lights[i]->x = 50;
		lights[i]->y = 50;		
	}
	spells = new_ptrarray();
	
	IsoLight *light = new_isolight();
	light->r = 1;
	light->g = 1;
	light->b = 1;
	light->x = 50;
	light->y = 50;
	light->range = 4;
	
	iso_set_ambient(0.05, 0.05, 0.05);	
	
	float time_step = 1.0 / 30.0;
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
				iso_screen2world(window_xmouse(),  window_ymouse(), &command.move_x, &command.move_y);
				network->add_command(network->state, (Netcmd*)&command);
			}
			if (window_mousepressed(1)) {								
				Netcmd_SpawnFlare cmd;
				cmd.header.type = NETCMD_SPAWNFLARE;
				cmd.sender = network->get_id(network->state);

				Critter *player = cri[cmd.sender];
				float hp = player->vtable->get_hp(player);

				if (hp >= 1) {
					player->vtable->get_viewpoint(player, &cmd.x, &cmd.y);									
					iso_screen2world(window_xmouse(),  window_ymouse(), &cmd.target_x, &cmd.target_y);
					network->add_command(network->state, (Netcmd*)&cmd);
				}
			}

			Netcmd *command;
			while ((command = network->get_command(network->state))) {
				switch (command->header.type) {
				case NETCMD_SETTILE:{
						command_set_tile(engine, (Netcmd_SetTile *) command);
						break;
					}
				case NETCMD_MOVECRITTER:{
						Netcmd_MoveCritter *move = (Netcmd_MoveCritter *) command;
						active[move->sender] = 1;
						cri[move->sender]->vtable->order(cri[move->sender], command);
						break;
					}
				case NETCMD_SPAWNFLARE:{
						Netcmd_SpawnFlare *sf = (Netcmd_SpawnFlare *) command;
						Spell *flare = new_flare(sf->x, sf->y, sf->target_x, sf->target_y);
						ptrarray_add(spells, flare);
						break;
					}				
				default:
					printf("Unknown command\n");
				}
				free(command);
			}

			for (int i = 0; i < MAX_CLIENTS; i++) {
				cri[i]->vtable->tick(cri[i]);				
			}			
			for (int i = 0; i < spells->count; i++) {
				Spell *spell = (Spell *)ptrarray(spells)[i];
				spell->vtable->tick(&spell);
				if (spell == NULL) {
					ptrarray_remove(spells, i);
					i--;
				}				
			}									

			network->logic_tick(network->state);						
			Critter *c = cri[network->get_id(network->state)];			
			c->vtable->get_viewpoint(c, &camera->x, &camera->y);			

			window_poll_events();
		}		

		draw_tilemap(engine->map, camera, g_tileset);

		for (int i = 0; i < MAX_CLIENTS; i++) {
			cri[i]->vtable->draw(cri[i], window_frame_time());
			if (active[i]) {
				cri[i]->vtable->get_viewpoint(cri[i], &lights[i]->x, &lights[i]->y);
				lights[i]->range = 2;
			} else {
				lights[i]->range = 0;
			}
		}
		for (int i = 0; i < spells->count; i++) {
			Spell *spell = (Spell *)ptrarray(spells)[i];
			spell->vtable->draw(spell, window_frame_time());
		}

		Critter *c = cri[network->get_id(network->state)];
		int hp = c->vtable->get_hp(c);

		font_print(font_get("Jura"), 10, 10, 1.0, "HP: %d\nFPS: %d", hp, (int)round(1.0 / window_frame_time()));

		window_end_frame();
	}

	network->cleanup(network->state);
	camera_free(camera);
	engine_free(engine);
	window_close();
}

void server_loop(NetworkType * network)
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
