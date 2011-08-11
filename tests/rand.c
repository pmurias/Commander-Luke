#include <stdio.h>
#include "rand.h"
int main() {
    int i;
    int tests=0;
    unsigned myseed;

    printf("1..10\n");
    unsigned int numbers[10];
    rand_set_seed(0x19610910);

    for (i = 0;i < 10;i++) {
      numbers[i] = rand_rand();
    }
    myseed = 0x19610910;
    for (i = 0; i < 3; i++)
    {
      for (i = 0;i<10;i++) {
        unsigned int n = rand_rand_r(&myseed);
        if (n == numbers[i]) {
          printf("ok %d\n",++tests);
        } else {
          printf("not ok %d # got: %d expected: %d\n",++tests,n,numbers[i]);
        }
      }
    }

    return 0;
}

