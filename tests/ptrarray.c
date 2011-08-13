#include <stdio.h>
#include <stdlib.h>

#include "array.h"

typedef struct Bulk
{
	char data[2048];
	float floats[1024];
} Bulk;

int main(void)
{
	PtrArray *arr = new_ptrarray();
	int i;
	while (1) 
	{		
		for (i = 0; i < 100; i++) {
			Bulk *bulk  = malloc(sizeof(Bulk));
			ptrarray_add(arr, bulk);
			bulk->data[0] = i;
		}
		
		for (i = 0; i < 100; i++) {
			Bulk *bulk = ptrarray(arr)[0];
			printf("%d", bulk->data[0]);
			free(bulk);
			ptrarray_remove(arr, 0);			
		}		
	}
	return 0;
}