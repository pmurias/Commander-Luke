#ifndef __SERIALIZERS_H__
#define __SERIALIZERS_H__

#define GENERIC_CORE_SERIALIZER(GENTYPE, TYPE) \
static void serialize(GENTYPE *g, void **buf, uint32_t *size) \
{ \
	TYPE *t = (TYPE*)g; \
	*buf = malloc(sizeof(TYPE##Core)); \
	memcpy(*buf, &t->c, sizeof(TYPE##Core)); \
	*size = sizeof(TYPE##Core); \
}

#define GENERIC_CORE_DESERIALIZER(GENTYPE, TYPE, rebuild) \
static void deserialize(GENTYPE *g, void *buf, uint32_t size) \
{ \
	TYPE *t = (TYPE*)g; \
	memcpy(&t->c, buf, sizeof(TYPE##Core)); \
	rebuild(g); \
}

#define GENERIC_CORE_PACK_SIZE(NAME, TYPE) \
uint32_t NAME(void) \
{ \
	return sizeof(TYPE##Core); \
}	

#endif

