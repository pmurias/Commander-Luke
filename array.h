#ifndef __ARRAY_H__
#define __ARRAY_H__

typedef struct Array
{
	void *data;
	int count;
	int size;
	int elem_size;
} Array;

Array *new_array(int elem_size);
void array_add(Array *arr, void *elem);
void *array_get(Array *arr, int i);
void array_remove(Array *arr, int i);
void array_clear(Array *arr);

#define PtrArray Array
#define new_ptrarray() new_array(sizeof(void*))
#define ptrarray_add(arr, a) array_add((arr), &(a))
#define ptrarray_remove array_remove
#define ptrarray_clear array_clear
void *ptrarray_get(Array *arr, int i);
void **ptrarray(Array *arr);

#endif // __ARRAY_H__