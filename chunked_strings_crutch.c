#include "chunked_strings_crutch.h"

void chunked_string_init(struct ChunkedString *chunked_string, size_t chunk_size) {
	chunked_string->data = NULL;
	chunked_string->allocated = 0;
	chunked_string->used = 0;
	chunked_string->chunk = chunk_size;
}
void chunked_string_free(struct ChunkedString *chunked_string) {
	free(chunked_string->data);
	chunked_string->allocated = 0;
	chunked_string->used = 0;
}
gchar *chunked_string_to_gchar(struct ChunkedString *chunked_string) {
	return chunked_string->data;
}

void chunked_string_add(struct ChunkedString *chunked_string, gchar *string,size_t length) {
	if (chunked_string == NULL)
		return;
	if (string == NULL)
		return;
	if (length == 0) {
		length = strlen(string);
	}
	chunked_string->used += length;
	if (chunked_string->used > chunked_string->allocated) {
		size_t diff, chunks, alloc_bytes;
		diff = chunked_string->used - chunked_string->allocated;
		chunks = diff / chunked_string->chunk;
		if (diff % chunked_string->chunk) chunks++;
		alloc_bytes = (chunked_string->allocated + (chunks * chunked_string->chunk)) * sizeof(gchar);
		chunked_string->data = realloc(chunked_string->data,alloc_bytes);
	}
	strncpy(chunked_string->data+chunked_string->used-length,string,length);
}

/* returning string must be freed by user */
gchar *chunked_string_replace(gchar *source, gchar *variable, gchar *replacement) {
	gchar *oldp = source;
	gchar *p = NULL;
	struct ChunkedString chunked_string;
	
	chunked_string_init(&chunked_string,1024);
	
	while ((p = strstr(oldp,variable))) {
		chunked_string_add(&chunked_string,oldp,(p-oldp));
		chunked_string_add(&chunked_string,replacement,0);
		oldp += (p-oldp);
		oldp += strlen(variable);
	}
	chunked_string_add(&chunked_string,oldp,0);
	return chunked_string_to_gchar(&chunked_string);
}
