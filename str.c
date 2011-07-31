#include "str.h"

#include <string.h>
#include <stdlib.h>

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
inline void str_set(Str *str, char *val)
{
	str->len = strlen(val);
	str->val = realloc(str->val, str->len + 1);
	strcpy(str->val, val);
	str->val[str->len] = 0;
	str->hash = str_hash(str->val, str->len);
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
