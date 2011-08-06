#include <GL/glfw.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "texture.h"
#include "socket.h"
#include "str.h"
#include "camera.h"

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
    if (x + tex->width >= 0 && x <= g_screenWidth && y + tex->height >= 0
	&& y <= g_screenHeight) {
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

	    if (tileX > 0 && tileY > 0 && tileX < map->width
		&& tileY < map->height) {
		Texture *tile =
		    tileset[map->tiles
			    [(int) (map->width * tileY + tileX)]];
		float scrX, scrY;
		tile_to_screen(tileX - tCamX, tileY - tCamY, &scrX, &scrY);
		blit(tile, round(offX + scrX + g_screenWidth / 2 - 80),
		     round(offY + scrY + g_screenHeight / 2 -
			   (tile->height - 40)));
	    }
	}
}

void opengl_start()
{
    glfwInit();
    glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 1);
    glfwOpenWindow(g_screenWidth, g_screenHeight, 8, 8, 8, 0, 0, 0,
		   GLFW_WINDOW);
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
    printf("Usage: luke [--server] [--client ip]");
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

typedef struct {
    void* data;
    void (*add_command)(void* data,int size,void *command);
    void* (*get_command)(void* data,int *size);
    void (*logic_tick)(void* data);
    void (*tick)(void* data);
} NetworkType;

typedef struct SingleCommand {
    struct SingleCommand* next;
    void* buf;
    int size;
} SingleCommand;
typedef struct {
  SingleCommand* last;
  SingleCommand* first;
  void* last_buf;
} SingleData;

void single_noop(void* data) {
}
void single_add_command(void* data,int size,void *buf) {
  SingleData* commands = (SingleData*) data;

  SingleCommand* c = (SingleCommand*) malloc(sizeof(SingleCommand)); 
  c->buf = malloc(size);
  memcpy(c->buf,buf,size);
  c->size = size;
  c->next = NULL;
  
  if (!commands->first) {
    commands->first = c;
    commands->last = c;
  } else {
    commands->last->next = c;
  }
}

void* single_get_command(void* data,int *size) {
  SingleData* commands = (SingleData*) data;
  if (commands->last_buf) {
    free(commands->last_buf);
    commands->last_buf = NULL;
  }
  if (!commands->first) {
    return NULL; 
  } else {
    SingleCommand* ret = commands->first;
    commands->first = ret->next;
    if (commands->last == ret) {
      commands->last = NULL;
    }
    *size = ret->size;
    commands->last_buf = ret->buf;
    return ret->buf;
  }
}

NetworkType* single_player() {
  NetworkType* single = malloc(sizeof(NetworkType));
  single->tick = single_noop;
  single->logic_tick = single_noop;
  single->add_command = single_add_command;
  single->get_command = single_get_command;

  SingleData* data = (SingleData*)malloc(sizeof(SingleData));
  single->data = (void*)data;
  data->last = NULL;
  data->first = NULL;
  data->last_buf = NULL;
  return single;
}

void command_set_tile(Engine* engine,int tileX,int tileY,int type) {
  engine->map->tiles[tileY * engine->map->width + tileX] = type;
}
void client_loop()
{

    opengl_start();
    load_assets();

    Engine *engine = engine_init();
    Camera *camera = camera_init();
    NetworkType* network = single_player();

    while (1) {
	network->tick(network->data);

	int mouseX, mouseY;
	glfwGetMousePos(&mouseX, &mouseY);

	if (glfwGetKey('X'))
	    break;


	if (glfwGetMouseButton(0)) {
	    float tileX, tileY;
	    snap_screen_to_tile(camera->x - g_screenWidth / 2 + mouseX,
				camera->y - g_screenHeight / 2 + mouseY,
				&tileX, &tileY);
	    if (tileX >= 0 && tileY >= 0) {
		int command[4];
		command[0] = 1;
		command[1] = tileX;
		command[2] = tileY;
		command[3] = 1;
		network->add_command(network->data,sizeof(int)*4,(void*)command);
	    }
	}

	network->logic_tick(network->data);
	int size;
	void* command;
	while ((command = network->get_command(network->data,&size))) {
	    switch (*((int*)command)) {
		case 1: {
		    int* as_ints = (int*)command;
		    int tileX = *(as_ints+1);
		    int tileY = *(as_ints+2);
		    int type = *(as_ints+3);
		    command_set_tile(engine,tileX,tileY,type);
		    break;
		}
		default:
		    printf("Unknown command\n");
	    }
	}

	camera_keyboard_control(camera);

	glClear(GL_COLOR_BUFFER_BIT);

	draw_tilemap(engine->map, camera, g_tileset);

	glfwSwapBuffers();
    }

    camera_free(camera);
    engine_free(engine);
    opengl_stop();
}

int main(int argc, char **argv)
{

    if (argc > 1) {
	if (strcmp(argv[1], "--server") == 0) {
	    printf("NYI\n");
	} else if (strcmp(argv[1], "--client") == 0 && argc == 3) {
	    printf("NYI\n");
	} else {
	    usage();
	}
    } else {
	client_loop();
    }


    return 0;
}
