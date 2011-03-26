#include "gui_crutch.h"

void toggle_checker(GtkCellRendererToggle *cell, gchar *path_str, gpointer data) {
	guint column_number;
	GtkTreeIter iter;
	GtkTreeModel *store = (GtkTreeModel *)data;
	gboolean checked;
	gboolean setter_checked;
	gboolean getter_checked;
	
	gtk_tree_model_get_iter_from_string(store,&iter,path_str);
	
	column_number = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(cell), "my_column_num"));
	
	gtk_tree_model_get(store,&iter,column_number,&checked,-1);
	gtk_tree_model_get(store,&iter,2,&setter_checked,-1);
	gtk_tree_model_get(store,&iter,3,&getter_checked,-1);

	
	checked ^= 1;

	gtk_tree_store_set((GtkTreeStore *)store,&iter,column_number,checked,-1);


	switch (column_number) {
		case 2:
			property_list.data[atoi(path_str)].do_setter = checked;
			if (!getter_checked && checked == FALSE) {
				gtk_tree_store_set((GtkTreeStore *)store,&iter,5,FALSE,-1);
			} else {
				gtk_tree_store_set((GtkTreeStore *)store,&iter,5,TRUE,-1);
			}
			break;
		case 3:
			property_list.data[atoi(path_str)].do_getter = checked;
			if (!setter_checked && checked == FALSE) {
					gtk_tree_store_set((GtkTreeStore *)store,&iter,5,FALSE,-1);
			} else {
				gtk_tree_store_set((GtkTreeStore *)store,&iter,5,TRUE,-1);
			}
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
	column = gtk_tree_view_column_new_with_attributes("Inner?",renderer,"active",IS_INNER,"sensitive",INNER_ENABLED,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	g_signal_connect (G_OBJECT(renderer), "toggled", G_CALLBACK(toggle_checker), store);
	
	renderer = gtk_cell_renderer_toggle_new();
	g_object_set(renderer,"visible", FALSE, NULL);
	column = gtk_tree_view_column_new_with_attributes("",renderer,"active",INNER_ENABLED,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
	
	
	return tree;
}

void populate_tree_with_data(GtkTreeStore *store) {
	GtkTreeIter iter;
	size_t i;
	
	for (i=0; i < property_list.used; i++) {
		gtk_tree_store_append(store,&iter,NULL);
		gtk_tree_store_set(store,&iter,PROP_TYPE,property_list.data[i].type,PROP_NAME,property_list.data[i].name,DO_SETTER,property_list.data[i].do_setter,DO_GETTER,property_list.data[i].do_getter,IS_INNER,property_list.data[i].is_inner,INNER_ENABLED,TRUE,-1);
	}
}

/* I'm not a GTK-man, so i do code crutches! */
void do_check_task(GtkButton *button, gpointer *user_data) {
	GtkTreeStore *store = (GtkTreeStore *)user_data;
	gint column_number;
	gboolean name_current_stright;
	gchar *name_stright;
	gchar *name_reverse;
	size_t i;
	GtkTreeIter iter;
	
	column_number = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button), "column-num"));
	name_current_stright = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button), "name-current-stright"));
	name_stright = g_object_get_data(G_OBJECT(button), "name-stright");
	name_reverse = g_object_get_data(G_OBJECT(button), "name-reverse");
		
	gtk_tree_model_get_iter_first((GtkTreeModel *)store,&iter);
	
	for (i=0; i < gtk_tree_model_iter_n_children((GtkTreeModel *)store,NULL); i++) {
		if (column_number >= 0) {
			gtk_tree_store_set((GtkTreeStore *)store,&iter,column_number,name_current_stright,-1);
			switch (column_number) {
				case 2:
					property_list.data[i].do_setter = name_current_stright;
					break;
				case 3:
					property_list.data[i].do_getter = name_current_stright;
					break;
				case 4:
					property_list.data[i].is_inner = name_current_stright;
					break;
			}
		} else {
			gtk_tree_store_set((GtkTreeStore *)store,&iter,DO_GETTER,name_current_stright,-1);
			gtk_tree_store_set((GtkTreeStore *)store,&iter,DO_SETTER,name_current_stright,-1);
			gtk_tree_store_set((GtkTreeStore *)store,&iter,IS_INNER,name_current_stright,-1);
			property_list.data[i].do_setter = name_current_stright;
			property_list.data[i].do_getter = name_current_stright;
			property_list.data[i].is_inner = name_current_stright;
		}
		gtk_tree_model_iter_next((GtkTreeModel *)store,&iter);
	}
	
	
	name_current_stright ^= 1;
	
	g_object_set_data(G_OBJECT(button), "name-current-stright",GINT_TO_POINTER(name_current_stright));
	
	
	if (name_current_stright) {
		g_object_set(button,"label", name_stright, NULL);
	} else {
		g_object_set(button,"label", name_reverse, NULL);
	}
}

gboolean gui_interaction_dialog() {
	gboolean res = FALSE;
	gboolean all_getters = TRUE;
	gboolean all_setters = TRUE;
	gboolean all_inner = TRUE; 
	gboolean all_all = TRUE;
	gboolean test;
	size_t i;
	GtkWidget *dialog, *vbox, *hbox1, *hbox2, *tree;
	GtkWidget *check_all_getters;
	GtkWidget *check_all_setters;
	GtkWidget *check_all_inner;
	GtkWidget *check_all;
	GtkTreeStore *store;
	
	gchar getters_stright[] = "All getters";
	gchar getters_reverse[] = "No getters";
	gchar setters_stright[] = "All setters";
	gchar setters_reverse[] = "No setters";
	gchar inner_stright[] = "All inner";
	gchar inner_reverse[] = "All outter";
	gchar all_stright[] = "Check ALL";
	gchar all_reverse[] = "Uncheck ALL";
	
	gchar *p = NULL;
	
	
	dialog = gtk_dialog_new_with_buttons("C++ Setters/Getters generator", GTK_WINDOW(geany->main_widgets->window),
										GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	hbox1 = gtk_hbox_new(TRUE,10);
	hbox2 = gtk_hbox_new(TRUE,10);
	gtk_widget_set_name(dialog, "cpp-setters-getters-generator");
	
	store = gtk_tree_store_new(N_COLS,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN,G_TYPE_BOOLEAN);

	tree = make_tree(store);
	
	populate_tree_with_data(store);
	
	test = TRUE;
	for (i=0; i < property_list.used; i++) {
		if (property_list.data[i].do_setter == FALSE || property_list.data[i].do_getter == FALSE || property_list.data[i].is_inner == FALSE) {
			test = FALSE;
			break;
		}
	}
	if (test) all_all = FALSE;
	
	test = TRUE;
	for (i=0; i < property_list.used; i++) {
		test = test && property_list.data[i].is_inner;
	}
	if (test) all_inner = FALSE;
	
	test = TRUE;
	for (i=0; i < property_list.used; i++) {
		test = test && property_list.data[i].do_setter;
	}
	if (test) all_setters = FALSE;
	
	test = TRUE;
	for (i=0; i < property_list.used; i++) {
		test = test && property_list.data[i].do_getter;
	}
	if (test) all_getters = FALSE;
	
		
	p = (all_getters) ? getters_stright : getters_reverse;
	check_all_getters = gtk_button_new_with_label(p);
	
	p = (all_setters) ? setters_stright : setters_reverse;
	check_all_setters = gtk_button_new_with_label(p);
	
	p = (all_inner) ? inner_stright : inner_reverse;
	check_all_inner = gtk_button_new_with_label(p);

	p = (all_all) ? all_stright : all_reverse;
	check_all = gtk_button_new_with_label(p);
	
	g_object_set_data(G_OBJECT(check_all_getters), "column-num", GINT_TO_POINTER(DO_GETTER));
	g_object_set_data(G_OBJECT(check_all_getters), "name-stright",getters_stright);
	g_object_set_data(G_OBJECT(check_all_getters), "name-reverse",getters_reverse);
	g_object_set_data(G_OBJECT(check_all_getters), "name-current-stright",GINT_TO_POINTER(all_getters));
	
	g_object_set_data(G_OBJECT(check_all_setters), "column-num", GINT_TO_POINTER(DO_SETTER));
	g_object_set_data(G_OBJECT(check_all_setters), "name-stright",setters_stright);
	g_object_set_data(G_OBJECT(check_all_setters), "name-reverse",setters_reverse);
	g_object_set_data(G_OBJECT(check_all_setters), "name-current-stright",GINT_TO_POINTER(all_setters));
	
	g_object_set_data(G_OBJECT(check_all_inner), "column-num", GINT_TO_POINTER(IS_INNER));
	g_object_set_data(G_OBJECT(check_all_inner), "name-stright",inner_stright);
	g_object_set_data(G_OBJECT(check_all_inner), "name-reverse",inner_reverse);
	g_object_set_data(G_OBJECT(check_all_inner), "name-current-stright",GINT_TO_POINTER(all_inner));
	
	g_object_set_data(G_OBJECT(check_all), "column-num", GINT_TO_POINTER(-1));
	g_object_set_data(G_OBJECT(check_all), "name-stright",all_stright);
	g_object_set_data(G_OBJECT(check_all), "name-reverse",all_reverse);
	g_object_set_data(G_OBJECT(check_all), "name-current-stright",GINT_TO_POINTER(all_all));
	
	g_signal_connect(G_OBJECT(check_all_setters), "clicked", G_CALLBACK(do_check_task), store);
	g_signal_connect(G_OBJECT(check_all_getters), "clicked", G_CALLBACK(do_check_task), store);
	g_signal_connect(G_OBJECT(check_all_inner), "clicked", G_CALLBACK(do_check_task), store);
	g_signal_connect(G_OBJECT(check_all), "clicked", G_CALLBACK(do_check_task), store);


	gtk_container_add(GTK_CONTAINER(hbox1), check_all_setters);
	gtk_container_add(GTK_CONTAINER(hbox1), check_all_getters);
	gtk_container_add(GTK_CONTAINER(hbox2), check_all_inner);
	gtk_container_add(GTK_CONTAINER(hbox2), check_all);

	gtk_container_add(GTK_CONTAINER(vbox), tree);
	gtk_container_add(GTK_CONTAINER(vbox), hbox1);
	gtk_container_add(GTK_CONTAINER(vbox), hbox2);
	
	gtk_widget_show_all (vbox);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		res = TRUE;
	}
	gtk_widget_destroy(dialog);
	return res;
}
