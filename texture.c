#include <png.h>
#include <stdlib.h>
#include <GL/gl.h>

#include "texture.h"

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void image_free(Image *img)
{
	free(img->pixels);	
}

//-----------------------------------------------------------------------------
int texture_from_image(Texture *tex, Image *img)
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

//-----------------------------------------------------------------------------
Texture *texture_from_file(char *fname)
{
	Image img;
	Texture *tex = malloc(sizeof(Texture));
	
	image_load_from_file(&img, fname);
	texture_from_image(tex, &img);
	image_free(&img);
	return tex;
}

//-----------------------------------------------------------------------------
void texture_bind(Texture *tex)
{
	static Texture *curr_tex = NULL;
	
	if (curr_tex != tex) {
		glBindTexture(GL_TEXTURE_2D, tex->handle);
		curr_tex = tex;
	}
}