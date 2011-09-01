#ifndef __STR_H__
#define __STR_H__

#include <stdint.h>

typedef struct
{
	char *val;
	uint32_t hash;
	uint16_t len;
} Str;

Str *new_str(void);
void str_init(Str *str);
void str_free(Str *str);
uint32_t str_hash(const char *val, uint16_t len);
void str_nset(Str *str, char *val, int n);
void str_set(Str *str, char *val);
void  str_cpy(Str *dst, Str *src);
uint32_t str_cmp(Str *str0, Str *str1);
void str_append(Str *dst, Str *str);
void str_nappend(Str *dst, char *val, int n);


#endif // __STR_H__
