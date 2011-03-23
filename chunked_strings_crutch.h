#ifndef CRUTCH_CHUNKED_STRINGS
#define CRUTCH_CHUNKED_STRINGS

#include <geanyplugin.h>
#include <string.h>
#include <stdlib.h>

struct ChunkedString {
	gchar *data;
	size_t allocated;
	size_t used;
	size_t chunk;
};

void chunked_string_init(struct ChunkedString *chunked_string, size_t chunk_size);
void chunked_string_free(struct ChunkedString *chunked_string);
gchar *chunked_string_to_gchar(struct ChunkedString *chunked_string);
void chunked_string_add(struct ChunkedString *chunked_string, gchar *string,size_t length);
gchar *chunked_string_replace(gchar *source, gchar *variable, gchar *replacement);
#endif
