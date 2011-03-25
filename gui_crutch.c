#include "gui_crutch.h"

void toggle_checker(GtkCellRendererToggle *cell, gchar *path_str, gpointer data) {
	guint column_number;
	GtkTreeIter iter;
	GtkTreeModel *store = (GtkTreeModel *)data;
	gboolean checked;
	
	gtk_tree_model_get_iter_from_string(store,&iter,path_str);
	column_number = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(cell), "my_column_num"));
	
	gtk_tree_model_get(store,&iter,column_number,&checked,-1);
	
	checked ^= 1;
	
	gtk_tree_store_set((GtkTreeStore *)store,&iter,column_number,checked,-1);

	
	switch (column_number) {
		case 2:
			property_list.data[atoi(path_str)].do_setter = checked;
			break;
		case 3:
			property_list.data[atoi(path_str)].do_getter = checked;
			break;
		case 4:
			property_list.data[atoi(path_str)].is_inner = checked;
			break;
	}
}

GtkWidget *make_tree(GtkTreeStore *store) {
	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	
	g_object_set(tree,"enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_VERTICAL, NULL);
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Type",renderer,"text",PROP_TYPE,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Property",renderer,"text",PROP_NAME,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(DO_SETTER));
	column = gtk_tree_view_column_new_with_attributes("Setter",renderer,"active",DO_SETTER,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	g_signal_connect (G_OBJECT(renderer), "toggled", G_CALLBACK(toggle_checker), store);
	
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(DO_GETTER));
	column = gtk_tree_view_column_new_with_attributes("Getter",renderer,"active",DO_GETTER,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	g_signal_connect (G_OBJECT(renderer), "toggled", G_CALLBACK(toggle_checker), store);
	
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set_data(G_OBJECT(renderer), "my_column_num", GUINT_TO_POINTER(IS_INNER));
	column = gtk_tree_view_column_new_with_attributes("Inner?",renderer,"active",IS_INNER,NULL);	
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	g_signal_connect (G_OBJECT(renderer), "toggled", G_CALLBACK(toggle_checker), store);
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("",renderer,"text",DUMMY,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	
	
	return tree;
}

void populate_tree_with_data(GtkTreeStore *store) {
	GtkTreeIter iter;
	size_t i;
	
	for (i=0; i < property_list.used; i++) {
		gtk_tree_store_append(store,&iter,NULL);
		gtk_tree_store_set(store,&iter,PROP_TYPE,property_list.data[i].type,PROP_NAME,property_list.data[i].name,DO_SETTER,property_list.data[i].do_setter,DO_GETTER,property_list.data[i].do_getter,IS_INNER,property_list.data[i].is_inner,-1);
	}
}

gboolean gui_interaction_dialog() {
	gboolean res = FALSE;
	GtkWidget *dialog, *vbox, *tree;
	GtkTreeStore *store;
	
	dialog = gtk_dialog_new_with_buttons("C++ Setters/Getters generator", GTK_WINDOW(geany->main_widgets->window),
										GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_widget_set_name(dialog, "cpp-setters-getters-generator");
	
	store = gtk_tree_store_new(N_COLS,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN,G_TYPE_STRING);

	tree = make_tree(store);
	
	populate_tree_with_data(store);

	gtk_container_add(GTK_CONTAINER(vbox), tree);
	
	gtk_widget_show_all (vbox);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		res = TRUE;
	}
	gtk_widget_destroy(dialog);
	return res;
}
