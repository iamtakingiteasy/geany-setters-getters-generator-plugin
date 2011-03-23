#ifndef GEANY_SETTERS_GETTERS_PLUGIN
#define GEANY_SETTERS_GETTERS_PLUGIN

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <geanyplugin.h>
#include <clang-c/Index.h>
#include "chunked_strings_crutch.h"
#include "chunked_clang_property_crutch.h"

/* plugin-specific things */

GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;

PLUGIN_VERSION_CHECK(147)

PLUGIN_SET_INFO("C++ setters/getters generator", "Generates C++ setters and getters methods",
                "0.03-alpha", "Alexander Tumin <itakingiteasy@gmail.com>")

static GtkWidget *main_menu_item = NULL;

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata);
void plugin_init(GeanyData *data);
void plugin_cleanup(void);

/* Default values of templates */

gchar sg_inner_template[] = "\
\tpublic:\n\
$SETTERS\n\
\tpublic:\n\
$GETTERS\n\
";
gchar sg_inner_setters[] = "\
\t\tvoid set_$NAME($TYPE value) {\n\
\t\t\tthis->$NAME = value;\n\
\t\t}\n\
";
gchar sg_inner_getters[] = "\
\t\t$TYPE get_$NAME(void) {\n\
\t\t\tthis->return this->$NAME;\n\
\t\t}\n\
";

/* utility variables */


/* utility functions */

#endif
