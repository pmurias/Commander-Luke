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
#include "ai.h"
#include "tilemap.h"
#include "map_gen.h"

#include "commands.h"
#include "camera.h"
#include "critter.h"
#include "critters/human.h"
#include "critters/blurred.h"
#include "spell.h"
#include "spells/flare.h"
#include "spells/nova.h"
#include "spells/teleport.h"


#define MAX_CLIENTS 20
IsoLight *lights[MAX_CLIENTS];
int active[MAX_CLIENTS];
Str *logins[MAX_CLIENTS];
uint32_t ticks;
char *login;

IntMap *critters;
uint32_t uid = 100;


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

void draw_walls(TileMap * map, Camera * cam, Sprite ** tileset)
{
	iso_set_cam(cam->x, cam->y);

	for (int i = 0; i < cam->view * 2 + 1; i++)
		for (int j = 0; j < cam->view + 1 - (i % 2); j++) {
			float tileX = round(cam->x) + i / 2 - j;
			float tileY = round(cam->y) + j + i / 2 + (i % 2) - cam->view;

			if (tileX >= 0 && tileY >= 0 && tileX < map->width && tileY < map->height) {
				int tid = map->wall_tiles[(int)(map->width * tileY + tileX)];
				if (tid & 2) isozbatch_add_sprite_off(tileset[1], tileX, tileY, -0.5, 0.5);
				if (tid & 4) isozbatch_add_sprite_off(tileset[2], tileX, tileY, 0.5, -0.5);
				if (tid & 8) isozbatch_add_sprite_off(tileset[3], tileX, tileY, -0.5, -0.5);
				if (tid & 16) isozbatch_add_sprite_off(tileset[4], tileX, tileY, -0.5, -0.5);
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
	Engine *engine = (Engine*)malloc(sizeof(Engine));
	engine->map = overworld_gen(100, 100);	
	return engine;
}

void engine_free(Engine * engine)
{
	tilemap_free(engine->map);
	free(engine);
}

Sprite *g_tileset[12];
Sprite *w_tileset[12];
void load_assets()
{
	g_tileset[0] = blit_load_sprite("./data/tiles/template.png");
	g_tileset[1] = blit_load_sprite("./data/tiles/grass0.png");
	g_tileset[2] = blit_load_sprite("./data/tiles/grass1.png");
	g_tileset[3] = blit_load_sprite("./data/tiles/grass2.png");
	g_tileset[4] = blit_load_sprite("./data/tiles/grass3.png");		
	
	w_tileset[1] = blit_load_sprite("./data/wall/wall001.png");
	w_tileset[2] = blit_load_sprite("./data/wall/wall002.png");
	w_tileset[3] = blit_load_sprite("./data/wall/wall003.png");
	w_tileset[4] = blit_load_sprite("./data/wall/wall004.png");
	w_tileset[1]->center_y = w_tileset[1]->height - 40;
	w_tileset[2]->center_y = w_tileset[1]->height - 40;
	w_tileset[3]->center_y = w_tileset[1]->height - 40;
	w_tileset[4]->center_y = w_tileset[1]->height - 40;	

	blit_load_spritesheet_split("./data/SheetNolty.png", "./data/SheetNolty.txt");
	blit_load_spritesheet_split("./data/Anomaly.png", "./data/Anomaly.txt");
	blit_load_spritesheet("./data/blurred.png", "./data/blurred.txt");
	
	blit_load_sprite("./data/flare.png")->blend_mode = BLEND_ADD;	

	font_load("./data/font/jura.png", "./data/font/jura.fnt");
	font_load("./data/font/ubuntu.png", "./data/font/ubuntu.fnt");

	printf("Building animations...");
	isoanim_build("Nolty.Idle", 10, 0.3);
	isoanim_set_center(isoanim_get("Nolty.Idle"), 64, 20);
	isoanim_build("Nolty.Running", 15, 0.03);
	isoanim_set_center(isoanim_get("Nolty.Running"), 64, 20);

	isoanim_build("Anomaly.Idle", 10, 0.3);
	isoanim_set_center(isoanim_get("Anomaly.Idle"), 64, 20);
	isoanim_build("Anomaly.Running", 15, 0.03);
	isoanim_set_center(isoanim_get("Anomaly.Running"), 64, 20);
	printf("OK\n");
}

void command_set_tile(Engine * engine, Netcmd_SetTile * c)
{
	engine->map->tiles[c->tile_y * engine->map->width + c->tile_x] = c->type;
}

void client_snapshot_callback(void *buf, uint32_t size)
{		
	printf("----------------------------------------------------\n");
	for (int i = 0; i <MAX_CLIENTS; i++)
	{		
		if (!logins[i]) {
			logins[i] = new_str();
		}				
		str_set(logins[i], (char*)buf+i*15);		
	}	
	int off = 300;
	/* ai_seed */
	memcpy(&ai_seed, buf+off, 4);
	off += 4;
	
	/* critters */	
	off += critters_deserialize(buf+off);
	off += spells_deserialize(buf+off);
}

void game_logic_tick(NetworkType *network)
{	
	Netcmd *command;
	while ((command = network->get_command(network->state))) {
		switch (command->header.type) {
		case NETCMD_SETTILE:{
				//command_set_tile(engine, (Netcmd_SetTile *) command);
				break;
			}
		case NETCMD_MOVECRITTER:{
				Netcmd_MoveCritter *move = (Netcmd_MoveCritter *) command;				
				active[move->sender] = 1;
				Critter *c = intmap_find(critters, move->sender+1);
				c->vtable->order(c, command);
				break;
			}
		case NETCMD_SPAWNFLARE:{
				Netcmd_SpawnFlare *sf = (Netcmd_SpawnFlare *) command;
				Spell *flare = create_flare(1, sf->x, sf->y, sf->target_x, sf->target_y);
				intmap_ins(spells, spell_uid++, flare);				
				break;
			}
		case NETCMD_SPAWNTELEPORT:{
				Netcmd_SpawnTeleport *sf = (Netcmd_SpawnTeleport *) command;
				Spell *flare = create_teleport(1, sf->caster, sf->x, sf->y, sf->target_x, sf->target_y);
				intmap_ins(spells, spell_uid++, flare);				
				break;
			}
		case NETCMD_SPAWNNOVA:{
				Netcmd_SpawnNova *sf = (Netcmd_SpawnNova *) command;
				Spell *nova = create_nova(sf->x, sf->y,sf->sender+1);
                                
				intmap_ins(spells, spell_uid++, nova);				
				break;
			}
		case NETCMD_SETLOGIN:{
				Netcmd_SetLogin *sl = (Netcmd_SetLogin*) command;
				if (!logins[sl->sender]) {
					logins[sl->sender] = new_str();
				}
				str_set(logins[sl->sender], sl->login);
				break;
			}
		default:
			printf("Unknown command\n");
		}
		free(command);
	}	

	for (int i = 0; i < spells->size; i++) {
		if (spells->keys[i]) {
			Spell *spell = (Spell *)spells->data[i];
			spell->vtable->tick(&spell);
			if (spell == NULL) {
				intmap_free_slot(spells, i);				
			}
		}
	}
	for (int i = 0; i < critters->size; i++) {
		if (critters->keys[i]) {
			Critter *c = critters->data[i];
			c->vtable->tick(c);
		}
	}
}

NetworkType *network_type;
void newturn_callback(void)
{
	if (ticks) {
		printf("Zryw w przod %d klatek\n", ticks);
	}
	while (ticks) {
		game_logic_tick(network_type);
		ticks--;
	}	
}

void client_loop(NetworkType * network)
{						
	window_open(800, 600, 0, "Commander Luke");
	load_assets();
	network_type = network;

	Engine *engine = engine_init();
	Camera *camera = camera_init();

	for (int i = 0; i < MAX_CLIENTS; i++) {
		lights[i] = new_isolight();
		lights[i]->r = 1;
		lights[i]->g = 1;
		lights[i]->b = 1;
		lights[i]->range = 0;
		lights[i]->x = 50;
		lights[i]->y = 50;
	}
	
	IsoLight *light = new_isolight();
	light->r = 1;
	light->g = 1;
	light->b = 1;
	light->x = 50;
	light->y = 50;
	light->range = 4;
	
	iso_set_ambient(0.05, 0.05, 0.05);

	Netcmd_SetLogin cmd;
	cmd.header.type = NETCMD_SETLOGIN;
	cmd.sender = network->get_id(network->state);		
	if (logins[cmd.sender]) {	/* in single player there is no login */
		memcpy(cmd.login, logins[cmd.sender]->val, logins[cmd.sender]->len+1);
		network->add_command(network->state, (Netcmd*)&cmd);				
	}
	
	float time_step = 1.0 / 30.0;
	float time_accum = 0;
	int running = 1;

        int weapon = 1;

	while (running) {
		window_start_frame();

		network->tick(network->state);
		time_accum += window_frame_time();
		while (time_accum >= time_step) {
			time_accum -= time_step;

			if (window_keypressed('1')) {
                        	weapon = 1;
                        }
			if (window_keypressed('2')) {
                        	weapon = 2;
                        }
			if (window_keypressed('3')) {
                        	weapon = 3;
                        }

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

			if (weapon == 1 && window_mousepressed(1)) {
					Netcmd_SpawnNova cmd;
					cmd.header.type = NETCMD_SPAWNNOVA;
					cmd.sender = network->get_id(network->state);
	
					Critter *player = intmap_find(critters, cmd.sender+1);
					float hp = player->vtable->get_hp(player);
	
					if (hp >= 1) {
						player->vtable->get_viewpoint(player, &cmd.x, &cmd.y);					
						network->add_command(network->state, (Netcmd*)&cmd);
					}
			}
			if (weapon == 2 && window_mousepressed(1)) {
				for (int i=0; i < 3; i++) {
					Netcmd_SpawnFlare cmd;
					cmd.header.type = NETCMD_SPAWNFLARE;
					cmd.sender = network->get_id(network->state);
	
					Critter *player = intmap_find(critters, cmd.sender+1);
					float hp = player->vtable->get_hp(player);
	
					if (hp >= 1) {
						player->vtable->get_viewpoint(player, &cmd.x, &cmd.y);					
						iso_screen2world(window_xmouse(),  window_ymouse(), &cmd.target_x, &cmd.target_y);
						float f = 200.0/sqrt(pow(cmd.x-cmd.target_x, 2) + pow(cmd.y-cmd.target_y, 2));
						cmd.target_x += (float)((rand_rand()%100)-50)/f;
						cmd.target_y += (float)((rand_rand()%100)-50)/f;
						network->add_command(network->state, (Netcmd*)&cmd);
					}
				}
			}
			if (weapon == 3 && window_mousepressed(1)) {
					Netcmd_SpawnTeleport cmd;
					cmd.header.type = NETCMD_SPAWNTELEPORT;
					cmd.sender = network->get_id(network->state);
					cmd.caster = network->get_id(network->state)+1;
	
					Critter *player = intmap_find(critters, cmd.sender+1);
					float hp = player->vtable->get_hp(player);
	
					if (hp >= 1) {
						player->vtable->get_viewpoint(player, &cmd.x, &cmd.y);					
						iso_screen2world(window_xmouse(),  window_ymouse(), &cmd.target_x, &cmd.target_y);
						float f = 200.0/sqrt(pow(cmd.x-cmd.target_x, 2) + pow(cmd.y-cmd.target_y, 2));
						cmd.target_x += (float)((rand_rand()%100)-50)/f;
						cmd.target_y += (float)((rand_rand()%100)-50)/f;
						network->add_command(network->state, (Netcmd*)&cmd);
					}
                        }
							
			if (ticks) {
				game_logic_tick(network);				
				ticks--;
			} else {
				printf("Zrywik w miejscu\n");
			}
			network->logic_tick(network->state);
																	
			Critter *c = intmap_find(critters, 1+network->get_id(network->state));			
			c->vtable->get_viewpoint(c, &camera->x, &camera->y);
						
			window_poll_events();
		}

		draw_tilemap(engine->map, camera, g_tileset);

		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (active[i]) {
				Critter *c = intmap_find(critters, i+1);
				c->vtable->get_viewpoint(c, &lights[i]->x, &lights[i]->y);
				lights[i]->range = 2;
			} else {
				lights[i]->range = 0;
			}
		}


		for (int i = 0; i < critters->size; i++) {
			if (critters->keys[i]) {
				Critter *c = critters->data[i];
				c->vtable->draw(c, window_frame_time());
			}
		}

		for (int i = 0; i < spells->size; i++) {
			if (spells->keys[i]) {
				Spell *spell = (Spell *)spells->data[i];
				spell->vtable->draw(spell, window_frame_time());
			}
		}

//		draw_walls(engine->map, camera, w_tileset);
		for (int i = 0; i < spells->size; i++) {
			if (spells->keys[i]) {
				Spell *spell = (Spell *)spells->data[i];
				spell->vtable->draw(spell, window_frame_time());
			}
		}
		isozbatch_draw();
		

		Critter *c = intmap_find(critters, network->get_id(network->state)+1);
		int hp = c->vtable->get_hp(c);

		font_print(font_get("Jura"), 10, 10, 1.0, "HP: %d\nFPS: %d", hp, (int)round(1.0 / window_frame_time()));
		for (int i = 0; i< MAX_CLIENTS; i++) {
			if (logins[i]!= NULL) {
				float x, y, sx, sy;
				Critter *c = intmap_find(critters, i+1);
				c->vtable->get_viewpoint(c, &x, &y);
				hp = c->vtable->get_hp(c);
				iso_world2screen(x, y, &sx, &sy);				
				int width = font_str_width(font_get("Jura"), 1.0, "%s\n%d", logins[i]->val, hp);
				font_print(font_get("Jura"), sx - (width*0.5), sy, 1.0, "%s\n%d", logins[i]->val, hp);
			}
		}

		window_end_frame();
	}

	network->cleanup(network->state);
	camera_free(camera);
	engine_free(engine);
	window_close();
}


void tcpclient_loop(NetworkType *network)
{
	tcpclientstate_set_snapshot_callback(network->state, &client_snapshot_callback);
	tcpclientstate_set_newturn_callback(network->state, &newturn_callback);
	tcpclientstate_login(network->state, login, 1+strlen(login));
	client_loop(network);
}

//------------------------------------------------------------------------------
void server_snapshot_callback(void **buf, uint8_t cid, uint32_t *size)
{	
	printf("----------------------------------------------------\n");
	*size = MAX_CLIENTS*15;
	*buf = malloc(*size);	
	memset(*buf, 0, *size);
	/* logins */	
	for (int i = 0; i < MAX_CLIENTS; i++) {		
		if (logins[i] != NULL) {								
			memcpy(*buf + i*15, logins[i]->val, logins[i]->len);			
		}
	}
	/* ai_seed */
	*buf = realloc(*buf, *size + 4);
	memcpy(*buf + *size, &ai_seed, 4);
	*size += 4;
	
	/* critters data */
	void *fragbuf;
	uint32_t fragsize;
	critters_serialize(&fragbuf, &fragsize);
	*buf = realloc(*buf, *size + fragsize);
	memcpy(*buf + *size, fragbuf, fragsize);
	free(fragbuf);
	*size += fragsize;	
	
	/* spells data */
	spells_serialize(&fragbuf, &fragsize);
	*buf = realloc(*buf, *size + fragsize);
	memcpy(*buf + *size, fragbuf, fragsize);
	free(fragbuf);
	*size += fragsize;	
}

int server_login_callback(void *login, uint8_t cid, uint32_t size)
{
	if (!logins[cid]) {
		logins[cid] = new_str();
	}
	str_set(logins[cid], (char*)login);
	return 1;
}

void server_loop(NetworkType * network)
{				
	network_type = network;	
	
	while (1) {
		network->tick(network->state);			
	}
}

void tcpserver_loop(NetworkType *network)
{
	tcpserverstate_set_snapshot_callback(network->state, &server_snapshot_callback);
	tcpserverstate_set_login_callback(network->state, &server_login_callback);
	tcpserverstate_set_turnsent_callback(network->state, &newturn_callback);
	server_loop(network);	
}

void system_startup()
{
	socket_startup();
	commands_startup();
	blit_startup();
	font_startup();
	iso_startup(160, 80);
	
	human_init_vtable();
	blurred_init_vtable();
	flare_init_vtable();
	nova_init_vtable();
	teleport_init_vtable();
	
	spells = new_intmap();
	critters = new_intmap();
	
	for (int i = 0; i < MAX_CLIENTS; i++) {
		Critter *c = create_human(1, 50, 50,0);			
		intmap_ins(critters, i+1, c);
	}
	
	/* npcs */
	for (int i=0;i<10;i++) {
		Critter *anomaly = create_human(1, 51,51,1);
		anomaly->vtable->set_ai(anomaly, AI_RUN_AROUND);		
		intmap_ins(critters, uid++, anomaly);
	}
}

int main(int argc, char **argv)
{
	system_startup();

	if (argc > 1) {
		if (strcmp(argv[1], "--server") == 0) {
			tcpserver_loop(new_tcp_server_state(&ticks));
		} else if (strcmp(argv[1], "--client") == 0 && argc == 4) {
			login = argv[3];
			tcpclient_loop(new_tcp_client_state(argv[2], 1234, &ticks));
		} else {
			usage();
		}
	} else if (argc == 1) {
		for (int i=0;i < MAX_CLIENTS;i++) {
			logins[i] = NULL;
		}
		client_loop(single_player_network(&ticks));
	} else {
		usage();
	}

	socket_startup();

	return 0;
}
