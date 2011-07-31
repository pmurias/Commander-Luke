#include <GL/glfw.h>
#include <png.h>
#include <stdlib.h>
#include <math.h>

#define NEWC(type, c) (type *)(malloc(sizeof(type) * (c)))

typedef struct
{
	unsigned char *pixels;
	unsigned int width;
	unsigned int height;
	unsigned int has_alpha;
} Image;
	
int image_load_from_file(Image *img, char *name) 
{
	png_structp pngPtr;
	png_infop infoPtr;
	unsigned int sigRead = 0;    
	FILE *fp;		

	if (!(fp = fopen(name, "rb")))
		return 0;

	pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	infoPtr = png_create_info_struct(pngPtr);   
	setjmp(png_jmpbuf(pngPtr));

	png_init_io(pngPtr, fp);    
	png_set_sig_bytes(pngPtr, sigRead);    
	png_read_png(pngPtr, infoPtr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, (void*)NULL);

	img->width = png_get_image_width(pngPtr, infoPtr);
	img->height = png_get_image_height(pngPtr, infoPtr);

	if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_RGBA)
		img->has_alpha = 1;
	else if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_RGB)
		img->has_alpha = 0;

	unsigned int rowBytes = png_get_rowbytes(pngPtr, infoPtr);
	img->pixels = (unsigned char*) malloc(rowBytes * img->height);

	png_bytepp rowPointers = png_get_rows(pngPtr, infoPtr);

	for (int i = 0; i < img->height; i++) 
		memcpy(img->pixels + rowBytes * i, rowPointers[i], rowBytes);

	png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);

	fclose(fp);

	return 1;
}

typedef struct
{
	unsigned int handle;
	unsigned int width;
	unsigned int height;
} Texture;

int texture_create_from_image(Texture *tex, Image *img)
{	
	tex->width = img->width;
	tex->height = img->height;
	glGenTextures(1, &tex->handle);
	glBindTexture(GL_TEXTURE_2D, tex->handle);
	glTexImage2D(GL_TEXTURE_2D, 0, img->has_alpha ? 4 : 3, img->width, img->height, 
		0, img->has_alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, img->pixels);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	return 1;
}

void blit(Texture *tex, int x, int y)
{
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
						
void draw_tilemap(TileMap *map, Camera *cam, Texture *tileset)
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
				Texture *tile = &tileset[map->tiles[(int)(map->width * tileY + tileX)]];
				float scrX, scrY;
				tile_to_screen(tileX - tCamX, tileY - tCamY, &scrX, &scrY);
				blit(tile, offX + scrX + 400 - 80, offY + scrY+300 - 40);
			}
		}
}

void start_opengl()
{
	glfwInit();	
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 1);
	glfwOpenWindow(800, 600, 8, 8, 8, 0, 0, 0, GLFW_WINDOW);
	glfwSetWindowTitle("Commander Luke");
	
	glOrtho(0, 800, 600, 0, -1, 1);
	glEnable(GL_TEXTURE_2D);	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
}

int main()
{
	start_opengl();
	
	Image images[3];
	Texture textures[3];
	Camera camera;
	TileMap map;
	
	camera.x = 0;
	camera.y = 0;
	camera.dx = 0;
	camera.dy = 0;
	camera.view = 9;
	
	init_tilemap(&map, 100, 100);
		
	image_load_from_file(&images[0], "./data/tiles/template.png");
	image_load_from_file(&images[1], "./data/tiles/grass1.png");
	image_load_from_file(&images[2], "./data/tiles/walln.png");		
	for (int i = 0; i < 3; i++) texture_create_from_image(&textures[i], &images[i]);

	while (1) 
	{
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
			int mouseX, mouseY;			
			float tileX, tileY;
			glfwGetMousePos(&mouseX, &mouseY);
			snap_screen_to_tile(camera.x - 400 + mouseX, camera.y - 300 + mouseY, &tileX, &tileY);
			map.tiles[(int)(tileY * map.width + tileX)] = 1;    
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
			
		draw_tilemap(&map, &camera, textures);
			
		glfwSwapBuffers();
	}

	glfwTerminate();
	return 0;
}