#include <stdlib.h>
#include <string.h>

#include "array.h"

#define MIN_ARRAY_SIZE 32

//-----------------------------------------------------------------------------
Array *new_array(int elem_size)
{
	Array *arr = malloc(sizeof(Array));
	arr->elem_size = elem_size;
	arr->size = MIN_ARRAY_SIZE;
	arr->data = malloc(MIN_ARRAY_SIZE * elem_size);
	arr->count = 0;
	return arr;
}

//-----------------------------------------------------------------------------
void array_add(Array *arr, void *elem)
{
	if (arr->count+1 == arr->size) {
		arr->size*= 2;
		arr->data = realloc(arr->data, arr->size*arr->elem_size);
	}
	memcpy(arr->data + arr->count*arr->elem_size, elem, arr->elem_size);
	arr->count++;
}

//-----------------------------------------------------------------------------
void *array_get(Array *arr, int i)
{
	return arr->data + i*arr->elem_size;
}

//-----------------------------------------------------------------------------
void array_remove(Array *arr, int i)
{
	if (i!= arr->count-1) {		
		memcpy(arr->data +i*arr->elem_size, arr->data + (i+1)*arr->elem_size, (arr->count-i-1)*arr->elem_size);
	}
	arr->count--;
	if (arr->count < arr->size/4 && arr->size/2 > MIN_ARRAY_SIZE) {
		arr->size /= 2;
		arr->data = realloc(arr->data, arr->size*arr->elem_size);
	}
}

//-----------------------------------------------------------------------------
void array_clear(Array *arr)
{
	arr->count = 0;
	arr->size = MIN_ARRAY_SIZE;
	arr->data = realloc(arr->data, arr->size * arr->elem_size);
}

//-----------------------------------------------------------------------------
void *ptrarray_get(Array *arr, int i)
{
	return *((void **)array_get(arr, i));
}

//-----------------------------------------------------------------------------
int ptrarray_free(Array *arr, void *ptr)
{
	for (int i=0; i<arr->count; i++) {
		if (ptrarray(arr)[i] == ptr) {
			free(ptr);
			array_remove(arr, i);
			return 1;			
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
void **ptrarray(Array *arr)
{
	return (void **)arr->data;
}
