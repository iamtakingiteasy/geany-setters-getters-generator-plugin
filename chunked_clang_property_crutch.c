#include "chunked_clang_property_crutch.h"

void chunked_property_init(struct PropertyList *chunked_property, size_t chunk_size) {
	if (chunked_property == NULL)
		return;
	chunked_property->data = NULL;
	chunked_property->allocated = 0;
	chunked_property->used = 0;
	chunked_property->chunk = 1;
}
void chunked_property_free(struct PropertyList *chunked_property) {
	size_t i;
	
	if (chunked_property == NULL)
		return;
	for (i=0; i < chunked_property->used; i++) {
		free(chunked_property->data[i].type);
		free(chunked_property->data[i].name);
		chunked_property->data[i].type = NULL;
		chunked_property->data[i].name = NULL;
	}
	free(chunked_property->data);
	chunked_property->data = NULL;
	chunked_property->allocated = 0;
	chunked_property->used = 0;
}
struct Property *chunked_property_get(struct PropertyList *chunked_property, size_t index) {
	if (chunked_property == NULL)
		return NULL;
	return &chunked_property->data[index];
}
void chunked_property_add(struct PropertyList *chunked_property, gchar *type, gchar *name, gboolean do_getter, gboolean do_setter, gboolean is_inner, enum PropertyKind kind) {
	size_t type_length;
	size_t name_length;
	
	if (chunked_property == NULL)
		return;
	if (type == NULL)
		return;
	if (name == NULL)
		return;

	type_length = strlen(type) + 1;
	name_length = strlen(name) + 1;
	
	chunked_property->used++;
	
	if (chunked_property->used > chunked_property->allocated) {
		chunked_property->allocated++;
		chunked_property->data = realloc(chunked_property->data,chunked_property->allocated * sizeof (struct Property));
		chunked_property->data[chunked_property->allocated-1].type = malloc((type_length + 1) * sizeof (gchar));
		chunked_property->data[chunked_property->allocated-1].name = malloc((name_length + 1) * sizeof (gchar));
		
		strncpy(chunked_property->data[chunked_property->allocated-1].type,type,type_length);
		strncpy(chunked_property->data[chunked_property->allocated-1].name,name,name_length);
		chunked_property->data[chunked_property->allocated-1].do_getter = do_getter;
		chunked_property->data[chunked_property->allocated-1].do_setter = do_setter;
		chunked_property->data[chunked_property->allocated-1].is_inner =  is_inner;
		chunked_property->data[chunked_property->allocated-1].kind = kind;
	}
}
