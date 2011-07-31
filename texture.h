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

extern int image_load_from_file(Image *img, char *name);

extern int texture_create_from_image(Texture *tex, Image *img);

#endif
