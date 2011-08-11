#ifndef __RAND_H__
#define __RAND_H__
int rand_rand_r(unsigned int *ctx);
int rand_rand(void);
void rand_set_seed(unsigned int seed);
#endif
