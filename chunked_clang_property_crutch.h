#ifndef CRUTCH_CHUNKED_CLANG_PROPS
#define CRUTCH_CHUNKED_CLANG_PROPS

#include <geanyplugin.h>
#include <stdlib.h>
#include <string.h>

enum PropertyKind {
	INVALID_KIND,
	PUBLIC,
	PRIVATE,
	PROTECTED
};

struct Property {
	gchar *type;
	gchar *name;
	enum PropertyKind kind;
};

struct PropertyList {
		struct Property *data;
		size_t allocated;
		size_t used;
		size_t chunk;
};

void chunked_property_init(struct PropertyList *chunked_property, size_t chunk_size);
void chunked_property_free(struct PropertyList *chunked_property);
struct Property *chunked_property_get(struct PropertyList *chunked_property, size_t index);
void chunked_property_add(struct PropertyList *chunked_property, gchar *type, gchar *name, enum PropertyKind kind);
#endif
