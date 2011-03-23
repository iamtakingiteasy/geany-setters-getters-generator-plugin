#include "setters_getters.h"

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata) {
	
}


void plugin_init(GeanyData *data) {
    main_menu_item = gtk_menu_item_new_with_mnemonic("Generate setters/getters");
    gtk_widget_show(main_menu_item);
    gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
        main_menu_item);
    g_signal_connect(main_menu_item, "activate",
        G_CALLBACK(item_activate_cb), NULL);
}

/*
GtkWidget* plugin_configure (GtkDialog *dialog)  {
	GtkWidget *vbox;
	vbox = gtk_vbox_new(FALSE, 6);
	return vbox;
}
*/

void plugin_cleanup(void) {
    gtk_widget_destroy(main_menu_item);
}
