#include "str.h"

#include <string.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
Str *new_str(void)
{
	Str *str = malloc(sizeof(Str));
	str_init(str);
	return str;
}

//-----------------------------------------------------------------------------
inline void str_init(Str *str)
{
	str->len = 0;
	str->val = malloc(1);
	str->val[0] = 0;
	str->hash = 0;
}

//-----------------------------------------------------------------------------
inline void str_free(Str *str)
{
	free(str->val);
	free(str);
}

//-----------------------------------------------------------------------------
inline uint32_t str_hash(const char *val, uint16_t len)
{
	/* djb2 hash */

	uint32_t hash = 5381;
	while (len > 0) {
		hash = (hash << 5) + hash + val[--len];
	}
	return hash;
}

//-----------------------------------------------------------------------------
inline void str_nset(Str *str, char *val, int n)
{
	str->len = n;
	str->val = realloc(str->val, str->len + 1);
	strncpy(str->val, val, n);
	str->val[str->len] = 0;
	str->hash = str_hash(str->val, str->len);
}

//-----------------------------------------------------------------------------
inline void str_set(Str *str, char *val)
{
	str_nset(str, val, strlen(val));
}

//-----------------------------------------------------------------------------
inline void str_cpy(Str *dst, Str *src)
{
	dst->val = realloc(dst->val, src->len + 1);
	strcpy(dst->val, src->val);
	dst->val[src->len] = 0;
	dst->hash = src->hash;
	dst->len = src->len;
}

//-----------------------------------------------------------------------------
inline uint32_t str_cmp(Str *str0, Str *str1)
{
	return ((str0->hash != str1->hash || str0->len != str1->len) ?
			0 : (strcmp(str0->val, str1->val) == 0));
}

//-----------------------------------------------------------------------------
void str_nappend(Str *dst, char *val, int n)
{
	dst->val = realloc(dst->val, dst->len + n + 1);
	strncpy(dst->val + dst->len, val, n);
	dst->len = dst->len + n;
	dst->val[dst->len] = 0;
	dst->hash = str_hash(dst->val, dst->len);
}

//-----------------------------------------------------------------------------
void str_append(Str *dst, Str *str)
{
	str_nappend(dst, str->val, str->len);
}
