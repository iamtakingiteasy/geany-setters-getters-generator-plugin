#ifndef CRUTCH_GUI
#define CRUTCH_GUI
#include "chunked_clang_property_crutch.h"
#include <geanyplugin.h>

extern GeanyPlugin     *geany_plugin;
extern GeanyData       *geany_data;
extern GeanyFunctions  *geany_functions;

extern struct PropertyList property_list;

struct CheckTask {
	GtkTreeStore *store;
	size_t column;
	gboolean value;
};

enum TreeCols {
	PROP_TYPE,
	PROP_NAME,
	DO_SETTER,
	DO_GETTER,
	IS_INNER,
	INNER_ENABLED,
	N_COLS
};

void toggle_checker(GtkCellRendererToggle *cell, gchar *path_str, gpointer data);
GtkWidget *make_tree(GtkTreeStore *store);
void populate_tree_with_data(GtkTreeStore *store);
gboolean gui_interaction_dialog();

#endif
