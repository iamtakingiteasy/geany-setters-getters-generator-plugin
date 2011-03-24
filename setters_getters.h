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
StashGroup      *group;
gchar           *config_filename = NULL;
gchar           *config_dirname = NULL;
PLUGIN_VERSION_CHECK(147)

PLUGIN_SET_INFO("C++ setters/getters generator", "Generates C++ setters and getters methods",
                "0.03-alpha", "Alexander Tumin <itakingiteasy@gmail.com>")

static GtkWidget *main_menu_item = NULL;

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata);
void plugin_init(GeanyData *data);
void plugin_cleanup(void);

gboolean sg_do_setters;
gboolean sg_do_getters;
enum PropertyKind sg_setter_kind;
enum PropertyKind sg_getter_kind;
gboolean sg_placement_inner;

/* Default values of templates */

/* there are two sets of templates - on for methods delared inside of
 * class - "inner"; and one for declared outside of class - "outter" one.
 */

/* there are six template variales
 * $SETTERS - setters methods code
 * $GETTERS - getters methods ode
 * 
 * $CLASS - class of property
 * $KIND - kind of property ('private', 'public' or 'protected')
 * $NAME - name of property
 * $TYPE - type of property
 */
 

 
gchar *sg_inner_template;

 /* inner templates */

gchar *sg_inner_setters;
gchar *sg_inner_getters;

/* outter templates */

gchar *sg_outter_forward_template;

gchar *sg_outter_forward_setters;
gchar *sg_outter_forward_getters;

gchar *sg_outter_template;
gchar *sg_outter_setters;
gchar *sg_outter_getters;


/* utility variables */
enum PropertyKind property_kind;
struct PropertyList property_list;

/* utility functions */
gboolean isedge(char current_char, char previous_char);
gchar *trim_left_string(gchar *string);
gchar *trim_right_string(gchar *string);
enum CXChildVisitResult property_list_builder(CXCursor cursor, CXCursor cursor_parent, CXClientData client_data);
enum PropertyKind property_helper_get_kind(ScintillaObject *current_doc_sci, CXSourceLocation code_source_location);
gchar *property_helper_get_type(ScintillaObject *current_doc_sci, CXSourceLocation code_source_location);
void gen_setters_getters(ScintillaObject *current_doc_sci, CXCursor class_cursor, gboolean placement_inner);
gchar *substitute_variables(gchar *source, gchar *type, gchar *name, gchar *kind, gchar *class);
#endif
