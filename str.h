#ifndef __STR_H__
#define __STR_H__

#include <stdint.h>

typedef struct
{
	char *val;
	uint32_t hash;
	uint16_t len;
} Str;

extern void str_init(Str *str);
extern void str_free(Str *str);
extern uint32_t str_hash(const char *val, uint16_t len);
extern void str_set(Str *str, char *val);
extern void  str_cpy(Str *dst, Str *src);
extern uint32_t str_cmp(Str *str0, Str *str1);


#endif // __STR_H__
