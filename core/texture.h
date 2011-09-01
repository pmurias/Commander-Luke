#ifndef __TEXTURE_H__
#define __TEXTURE_H__

typedef struct
{
	unsigned char *pixels;
	unsigned int width;
	unsigned int height;
	unsigned int has_alpha;
} Image;

typedef struct
{
	unsigned int handle;
	int width;
	int height;
} Texture;

int image_load_from_file(Image *img, char *name);
void image_free(Image *img);

Texture *texture_from_image(Image *img);
Texture *texture_from_image_part(Image *img, int u, int v, int w, int h);
Texture* texture_from_file(char* name);
void texture_bind(Texture *tex);


#endif
