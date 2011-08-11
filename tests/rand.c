#include <stdio.h>
#include "rand.h"
int main() {
    int i;
    unsigned myseed;

    printf("seeding rand with 0x19610910: \n");
    rand_set_seed(0x19610910);

    printf("generating three pseudo-random numbers:\n");
    for (i = 0; i < 3; i++)
    {
	printf("next random number = %d\n", rand_rand());
    }

    printf("generating the same sequence with rand_r:\n");
    myseed = 0x19610910;
    for (i = 0; i < 3; i++)
    {
	printf("next random number = %d\n", rand_rand_r(&myseed));
    }

    return 0;
}

