#include "setters_getters.h"

gboolean isedge(char current_char, char previous_char) {
	if (current_char == ';' || current_char == ':' || current_char == '{' || current_char == '}')
		return TRUE;
	if (current_char == '\n' && previous_char != '\\')
		return TRUE;
	return FALSE;
}

gchar *trim_left_string(gchar *string) {
	size_t length = strlen(string) + 1;
	size_t begining = 0;
	gchar *trimmed = NULL;
	
	while(isspace(string[begining++]));
	trimmed = malloc((length-begining) * sizeof (char));
	strncpy(trimmed,(string+begining-1),length-begining+1);
	return trimmed;
}

enum PropertyKind property_helper_get_kind(ScintillaObject *current_doc_sci, CXSourceLocation code_source_location) {
	gchar *kind_keyword = NULL;
	unsigned int source_offset;
	enum PropertyKind property_kind = INVALID_KIND;
	
	clang_getInstantiationLocation(code_source_location,NULL,NULL,NULL,&source_offset);
	kind_keyword = sci_get_contents_range(current_doc_sci,source_offset,source_offset+9);
		
	if (strncmp(kind_keyword,"protected",9) == 0) {
		property_kind = PROTECTED;
	} else if (strncmp(kind_keyword,"private",7) == 0) {
		property_kind = PRIVATE;
	} else if (strncmp(kind_keyword,"public",6) == 0) {
		property_kind = PUBLIC;
	}
	
	free(kind_keyword);
	return property_kind;
}

gchar *property_helper_get_type(ScintillaObject *current_doc_sci, CXSourceLocation code_source_location) {
	gchar *type_string = NULL;
	gchar current_char, previous_char;
	unsigned int source_offset;
	size_t type_literal_begining = 0;
	size_t type_literal_ending = 0;
	
	gboolean is_edge = FALSE;
	gboolean is_in_type = FALSE;
	
	clang_getInstantiationLocation(code_source_location,NULL,NULL,NULL,&source_offset);
	
	while (is_edge == FALSE) {
		while (isspace((current_char = sci_get_char_at(current_doc_sci,--source_offset))));
		if (current_char == ',' || current_char == '=') {
			while (isspace((current_char = sci_get_char_at(current_doc_sci,--source_offset))));
			while (!isspace((current_char = sci_get_char_at(current_doc_sci,--source_offset))));
			continue;
		}
		
		previous_char = sci_get_char_at(current_doc_sci,source_offset-1);
		if (isedge(current_char,previous_char)) {
			if (type_string == NULL) {
				if (is_in_type == FALSE) {
					type_string = malloc(4 * sizeof(gchar));
					strncpy(type_string,"int",4);
				} else {
					gchar *untrimmed_type_string = NULL;
					
					type_literal_begining = source_offset + 1;
					untrimmed_type_string = sci_get_contents_range(current_doc_sci,type_literal_begining,type_literal_ending);
					type_string = trim_left_string(untrimmed_type_string);
					free (untrimmed_type_string);
				}
				is_edge = TRUE;
			}
		}
		if (is_in_type == FALSE) {
			type_literal_ending = source_offset + 1;
			is_in_type = TRUE;
		}
	}
	return type_string;
}


enum CXChildVisitResult property_list_builder(CXCursor cursor, CXCursor cursor_parent, CXClientData client_data) {
	ScintillaObject *current_doc_sci = (ScintillaObject *)client_data;
	enum CXCursorKind code_cursor_kind;
	CXSourceLocation code_source_location;
	
	code_source_location = clang_getCursorLocation(cursor);
	code_cursor_kind = clang_getCursorKind(cursor);
	
	if (code_cursor_kind == 1) {
		property_kind = property_helper_get_kind(current_doc_sci,code_source_location);
	} else if (code_cursor_kind == 6) {
		gchar *type = NULL;
		gchar *name = NULL;
		
		type = property_helper_get_type(current_doc_sci,code_source_location);
		name = (gchar *)clang_getCString(clang_getCursorSpelling(cursor));
		
		chunked_property_add(&property_list, type, name, sg_do_getters, sg_do_setters, sg_placement_inner, property_kind);
		
		free(type);
		free(name);
	}
	
	return CXChildVisit_Continue;
}

enum CXChildVisitResult filterer(CXCursor cursor, CXCursor cursor_parent, CXClientData client_data) {
	struct GcharTuple *tuple = (struct GcharTuple *)client_data;
	gchar *gettername = tuple->first;
	gchar *settername = tuple->second;
	gchar *classname = tuple->class_name;
	size_t number = tuple->number;
	
	enum CXCursorKind code_cursor_kind;
	enum CXCursorKind parent_cursor_kind;
	gchar *source_name = NULL;

	code_cursor_kind = clang_getCursorKind(cursor);
	parent_cursor_kind = clang_getCursorKind(cursor_parent);

	if (code_cursor_kind == 4) {
		source_name = (gchar *)clang_getCString(clang_getCursorSpelling(cursor));		
		if (strncmp(source_name,classname,strlen(classname)) == 0) {
			free(source_name);
			source_name = NULL;
			return CXChildVisit_Recurse;
		}
		free(source_name);
		source_name = NULL;
	}
	if (code_cursor_kind == 21) {
		source_name = (gchar *)clang_getCString(clang_getCursorSpelling(cursor));		
		if (strncmp(source_name,settername,strlen(settername)) == 0) {
			property_list.data[number].do_setter = FALSE;
			property_list.data[number].is_inner = FALSE;
		}
		if (strncmp(source_name,gettername,strlen(gettername)) == 0) {
			property_list.data[number].do_getter = FALSE;
			property_list.data[number].is_inner = FALSE;
		}
		free(source_name);
		source_name = NULL;
	}
	return CXChildVisit_Continue;
}

void filter_already_existing_methods(CXTranslationUnit code_translation_unit,gchar *filename, gchar *class_name) {
	size_t i;
	CXFile code_file;
	CXCursor code_cursor;
	CXSourceLocation code_source_location;
	gchar *settername;
	gchar *gettername;
	struct GcharTuple tuple;
		
	code_file = clang_getFile(code_translation_unit,filename);
	code_source_location = clang_getLocation(code_translation_unit,code_file,1,1);
	code_cursor = clang_getTranslationUnitCursor(code_translation_unit);
	
	for (i=0; i < property_list.used; i++) {
		gettername = malloc((strlen(sg_method_name_getter) + 1) * sizeof(gchar));
		strncpy(gettername,sg_method_name_getter,strlen(sg_method_name_getter) + 1);
		gettername = chunked_string_replace(gettername,"$NAME",property_list.data[i].name);
		
		settername = malloc((strlen(sg_method_name_setter) + 1) * sizeof(gchar));
		strncpy(settername,sg_method_name_setter,strlen(sg_method_name_setter) + 1);
		settername = chunked_string_replace(settername,"$NAME",property_list.data[i].name);
		
		tuple.first = gettername;
		tuple.second = settername;
		tuple.class_name = class_name;
		tuple.number = i;

		clang_visitChildren(code_cursor,filterer,&tuple);
		
		free(gettername);
		gettername = NULL;
		free(settername);
		settername = NULL;
	}
}

gchar *substitute_variables(gchar *source, gchar *methname, gchar *type, gchar *name, gchar *kind, gchar *class) {
	gchar *result = NULL;
	result = malloc((strlen(source) + 1) * sizeof(gchar));
	strncpy(result,source,strlen(source) + 1);
	result = chunked_string_replace(result,"$METHNAME",methname);
	result = chunked_string_replace(result,"$TYPE",type);
	result = chunked_string_replace(result,"$NAME",name);
	result = chunked_string_replace(result,"$KIND",kind);
	result = chunked_string_replace(result,"$CLASS",class);	
	return result;
}

gboolean gen_setters_getters(ScintillaObject *current_doc_sci, CXCursor class_cursor, gchar *class_name) {
	size_t i;
	CXSourceRange class_cursor_range;
	CXSourceLocation class_cursor_end;
	
	unsigned int class_end_point = 0;
	gchar c;
	gchar *tmp = NULL;
	gboolean do_setters = FALSE;
	gboolean do_getters = FALSE;
	gboolean was_inner = FALSE;
	gboolean was_outter = FALSE;
	
	struct ChunkedString inner;
	struct ChunkedString outter;
	
	struct ChunkedString inner_setters;
	struct ChunkedString inner_getters;	
		
	struct ChunkedString outter_forward_setters;
	struct ChunkedString outter_forward_getters;
	struct ChunkedString outter_setters;
	struct ChunkedString outter_getters;
		
	chunked_string_init(&inner,1024);
	chunked_string_init(&outter,1024);
	
	chunked_string_init(&inner_setters,1024);
	chunked_string_init(&inner_getters,1024);
	
	chunked_string_init(&outter_forward_getters,1024);
	chunked_string_init(&outter_forward_setters,1024);
	chunked_string_init(&outter_setters,1024);
	chunked_string_init(&outter_getters,1024);

	for (i=0; i < property_list.used; i++) {
		if (property_list.data[i].is_inner == TRUE) {
			if (was_inner == FALSE)
				was_inner = TRUE;
			if (property_list.data[i].do_setter == TRUE) {
				if (do_setters == FALSE)
					do_setters = TRUE;
				tmp = substitute_variables(sg_inner_setters,sg_method_name_setter,property_list.data[i].type,property_list.data[i].name,"public",class_name);
				chunked_string_add(&inner_setters,tmp,0);
				free(tmp);
				tmp = NULL;
			}
			if (property_list.data[i].do_getter == TRUE) {
				if (do_getters == FALSE)
					do_getters = TRUE;
				tmp = substitute_variables(sg_inner_getters,sg_method_name_getter,property_list.data[i].type,property_list.data[i].name,"public",class_name);
				chunked_string_add(&inner_getters,tmp,0);
				free(tmp);
				tmp = NULL;
			}
		} else {
			if (was_outter == FALSE)
				was_outter = TRUE;
			if (property_list.data[i].do_setter == TRUE) {
				if (do_setters == FALSE)
					do_setters = TRUE;
				tmp = substitute_variables(sg_outter_setters,sg_method_name_setter,property_list.data[i].type,property_list.data[i].name,"public",class_name);
				chunked_string_add(&outter_setters,tmp,0);
				free(tmp);
				tmp = NULL;
				tmp = substitute_variables(sg_outter_forward_setters,sg_method_name_setter,property_list.data[i].type,property_list.data[i].name,"public",class_name);
				chunked_string_add(&outter_forward_setters,tmp,0);
				free(tmp);
				tmp = NULL;
				
			}
			if (property_list.data[i].do_getter == TRUE) {
				if (do_getters == FALSE)
					do_getters = TRUE;
				tmp = substitute_variables(sg_outter_getters,sg_method_name_getter,property_list.data[i].type,property_list.data[i].name,"public",class_name);
				chunked_string_add(&outter_getters,tmp,0);
				free(tmp);
				tmp = NULL;
				tmp = substitute_variables(sg_outter_forward_getters,sg_method_name_getter,property_list.data[i].type,property_list.data[i].name,"public",class_name);
				chunked_string_add(&outter_forward_getters,tmp,0);
				free(tmp);
				tmp = NULL;
			}
		}
	}
	
	
	if (was_inner == TRUE) {
		if (chunked_string_to_gchar(&inner_setters) != NULL || chunked_string_to_gchar(&inner_getters) != NULL) {
			tmp = malloc((strlen(sg_inner_template) + 1) * sizeof(gchar));
			strncpy(tmp,sg_inner_template,strlen(sg_inner_template) + 1);	
			tmp = chunked_string_replace(tmp,"$SETTERS",chunked_string_to_gchar(&inner_setters));
			tmp = chunked_string_replace(tmp,"$GETTERS",chunked_string_to_gchar(&inner_getters));
			chunked_string_add(&inner,tmp,0);
			free(tmp);
			tmp = NULL;
		}
	}
	if (was_outter) {
		if (chunked_string_to_gchar(&outter_forward_setters) != NULL || chunked_string_to_gchar(&outter_forward_getters) != NULL) {
			tmp = malloc((strlen(sg_outter_forward_template) + 1) * sizeof(gchar));
			strncpy(tmp,sg_outter_forward_template,strlen(sg_outter_forward_template) + 1);
			tmp = chunked_string_replace(tmp,"$SETTERS",chunked_string_to_gchar(&outter_forward_setters));
			tmp = chunked_string_replace(tmp,"$GETTERS",chunked_string_to_gchar(&outter_forward_getters));
			chunked_string_add(&inner,tmp,0);
			free(tmp);
			tmp = NULL;
		}
		if (chunked_string_to_gchar(&outter_setters) != NULL || chunked_string_to_gchar(&outter_getters) != NULL) {
			tmp = malloc((strlen(sg_outter_template) + 1) * sizeof(gchar));
			strncpy(tmp,sg_outter_template,strlen(sg_outter_template) + 1);
			tmp = chunked_string_replace(tmp,"$SETTERS",chunked_string_to_gchar(&outter_setters));
			tmp = chunked_string_replace(tmp,"$GETTERS",chunked_string_to_gchar(&outter_getters));
			chunked_string_add(&outter,tmp,0);
			free(tmp);
			tmp = NULL;
		}
	}
	
	class_cursor_range = clang_getCursorExtent(class_cursor);
	class_cursor_end = clang_getRangeEnd(class_cursor_range);
	clang_getInstantiationLocation(class_cursor_end,NULL,NULL,NULL,&class_end_point);
	
	while ((c = sci_get_char_at(current_doc_sci,class_end_point++)) != ';') {
		if (!isspace(c)) {
			return FALSE;
		}
	}
	
	sci_insert_text(current_doc_sci,class_end_point,chunked_string_to_gchar(&outter));

	clang_getInstantiationLocation(class_cursor_end,NULL,NULL,NULL,&class_end_point);
	class_end_point -= 1;
	sci_insert_text(current_doc_sci,class_end_point,chunked_string_to_gchar(&inner));

	chunked_string_free(&inner);
	chunked_string_free(&outter);
	
	chunked_string_free(&inner_setters);
	chunked_string_free(&inner_getters);

	chunked_string_free(&outter_forward_getters);
	chunked_string_free(&outter_forward_setters);
	chunked_string_free(&outter_setters);
	chunked_string_free(&outter_getters);
	
	return TRUE;
}
static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata) {
	/* geany stuff */
	GeanyDocument *current_doc = NULL;
	ScintillaObject *current_doc_sci = NULL;
	size_t current_doc_length;
	size_t current_doc_position;
	size_t current_doc_line;
	size_t current_doc_column;
	gchar *current_doc_contents;
	gboolean res;
	/* clang stuff */
	gchar filename[] = "/path/to/nowhere/NonExistingFile.cpp";
	CXFile code_file;
	CXIndex code_index;
	CXCursor code_cursor;
	enum CXCursorKind code_cursor_kind;
	CXSourceLocation code_source_location;
	CXTranslationUnit code_translation_unit;
	struct CXUnsavedFile code_unsaved_file;
	gchar *class_name;
	CXString cx_class_name;

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	
	/* geany stuff */
	current_doc = document_get_current();
	current_doc_sci = current_doc->editor->sci;
	current_doc_length = sci_get_length(current_doc_sci);
	current_doc_position = sci_get_current_position(current_doc_sci);
	current_doc_line = sci_get_current_line(current_doc_sci) + 1;
	current_doc_column = sci_get_col_from_position(current_doc_sci,current_doc_position) + 1;
	current_doc_contents = sci_get_contents(current_doc_sci,current_doc_length);
	
	/* clang stuff */
	code_index = clang_createIndex(0,0);
	/* setting up unsaved struct */
	code_unsaved_file.Filename = filename;
	code_unsaved_file.Contents = current_doc_contents;
	code_unsaved_file.Length = current_doc_length;
	/* back to clang stuff */
	code_translation_unit = clang_parseTranslationUnit(code_index,filename,NULL,0,&code_unsaved_file,1,CXTranslationUnit_Incomplete);
	code_file = clang_getFile(code_translation_unit,filename);
	code_source_location = clang_getLocation(code_translation_unit,code_file,current_doc_line,current_doc_column);
	code_cursor = clang_getCursor(code_translation_unit,code_source_location);
	code_cursor_kind = clang_getCursorKind(code_cursor);

	while (code_cursor_kind != 70 && code_cursor_kind != 4) {
		code_cursor = clang_getCursorLexicalParent(code_cursor);
		code_cursor_kind = clang_getCursorKind(code_cursor);
	}
	
	if (code_cursor_kind == 70) {
		dialogs_show_msgbox(GTK_MESSAGE_INFO, "Not inside the class. Aborting.");
		return;
	}
	
	property_kind = PRIVATE;
	
	clang_visitChildren(code_cursor,property_list_builder,current_doc_sci);
	
	if (property_list.used == 0) {
		dialogs_show_msgbox(GTK_MESSAGE_INFO, "No properties in class. Aborting.");
		return;
	}
	
	cx_class_name = clang_getCursorSpelling(code_cursor);
	class_name = (gchar *)clang_getCString(cx_class_name); /*  does not need to be freed ... */	
	
	filter_already_existing_methods(code_translation_unit,filename,class_name);
	
	res = TRUE;
	if (sg_show_interaction) {
		res = gui_interaction_dialog();
	}

	if (res) {
		if (!gen_setters_getters(current_doc_sci,code_cursor,class_name)) {
			dialogs_show_msgbox(GTK_MESSAGE_INFO, "Class seems to be not terminated with ';' (semicolon) character. Aborting.");
		}
	}
		
	clang_disposeString(cx_class_name);
	chunked_property_free(&property_list);
}


void plugin_init(GeanyData *data) {
	config_filename = NULL;
	config_dirname = NULL;
	
	main_menu_item = gtk_menu_item_new_with_mnemonic("Generate setters/getters");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
		main_menu_item);
	g_signal_connect(main_menu_item, "activate",
		G_CALLBACK(item_activate_cb), NULL);
	
	config_filename = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S,"plugins", G_DIR_SEPARATOR_S, "setters-getters", G_DIR_SEPARATOR_S, "setters-getters.conf", NULL);
	config_dirname = g_path_get_dirname(config_filename);	
	if (!g_file_test(config_dirname, G_FILE_TEST_IS_DIR)) {
		utils_mkdir(config_dirname, TRUE);
	}
		
	group = stash_group_new("setters-getters");
	
	
	
	stash_group_add_string(group, &sg_method_name_getter, "sg_method_name_getter", "get_$NAME");
	stash_group_add_string(group, &sg_method_name_setter, "sg_method_name_setter", "set_$NAME");
	
	stash_group_add_string(group, &sg_inner_template, "sg_inner_template", "\tpublic:\n$SETTERS\n\tpublic:\n$GETTERS\n");
	stash_group_add_string(group, &sg_inner_setters, "sg_inner_setters", "\t\tvoid $METHNAME($TYPE value) {\n\t\t\tthis->$NAME = value;\n\t\t}\n");
	stash_group_add_string(group, &sg_inner_getters, "sg_inner_getters", "\t\t$TYPE $METHNAME(void) {\n\t\t\treturn this->$NAME;\n\t\t}\n");
	
	stash_group_add_string(group, &sg_outter_forward_template, "sg_outter_forward_template", "\tpublic:\n$SETTERS\n\tpublic:\n$GETTERS\n");
	stash_group_add_string(group, &sg_outter_forward_setters, "sg_outter_forward_setters", "\t\tvoid $METHNAME($TYPE value);\n");
	stash_group_add_string(group, &sg_outter_forward_getters, "sg_outter_forward_getters", "\t\t$TYPE $METHNAME(void);\n");
	stash_group_add_string(group, &sg_outter_template, "sg_outter_template", "\n$SETTERS\n$GETTERS\n");
	stash_group_add_string(group, &sg_outter_setters, "sg_outter_setters", "void $CLASS::$METHNAME($TYPE value) {\n\tthis->$NAME = value;\n}\n");
	stash_group_add_string(group, &sg_outter_getters, "sg_outter_getters", "$TYPE $CLASS::$METHNAME(void) {\n\tthis->return this->$NAME;\n}\n");
	
	stash_group_add_boolean(group, &sg_show_interaction, "sg_show_interaction", TRUE);
	stash_group_add_boolean(group, &sg_placement_inner, "sg_placement_inner", TRUE);
	stash_group_add_boolean(group, &sg_do_getters, "sg_do_getters", TRUE);
	stash_group_add_boolean(group, &sg_do_setters, "sg_do_setters", TRUE);

	stash_group_load_from_file(group,config_filename);
	stash_group_save_to_file(group, config_filename, G_KEY_FILE_NONE);
}

gchar *response_configure_string_helper(GtkDialog *dialog,gchar *propname) {
	GtkTextBuffer *buffer;
	GtkTextIter iter_start;
	GtkTextIter iter_end;	
	buffer = gtk_text_view_get_buffer(g_object_get_data(G_OBJECT(dialog), propname));
	gtk_text_buffer_get_start_iter(buffer,&iter_start);;
	gtk_text_buffer_get_end_iter(buffer,&iter_end);
	
	return gtk_text_buffer_get_text(buffer,&iter_start,&iter_end,FALSE);
	
}

void response_configure(GtkDialog *dialog, gint response, gpointer user_data) {
	if (response != GTK_RESPONSE_OK && response != GTK_RESPONSE_APPLY) {
		return;
	}
	sg_show_interaction = gtk_toggle_button_get_active(g_object_get_data(G_OBJECT(dialog), "sg_show_interaction"));
	sg_placement_inner = gtk_toggle_button_get_active(g_object_get_data(G_OBJECT(dialog), "sg_placement_inner"));
	sg_do_getters = gtk_toggle_button_get_active(g_object_get_data(G_OBJECT(dialog), "sg_do_getters"));
	sg_do_setters = gtk_toggle_button_get_active(g_object_get_data(G_OBJECT(dialog), "sg_do_setters"));

	free(sg_method_name_getter);
	free(sg_method_name_setter);
	free(sg_inner_template);
	free(sg_inner_setters);
	free(sg_inner_getters);
	
	free(sg_outter_forward_template);
	free(sg_outter_forward_setters);
	free(sg_outter_forward_getters);
	free(sg_outter_template);
	free(sg_outter_setters);
	free(sg_outter_getters);
	
	
	sg_method_name_getter = response_configure_string_helper(dialog,"sg_method_name_getter");
	sg_method_name_setter = response_configure_string_helper(dialog,"sg_method_name_setter");
	sg_inner_template = response_configure_string_helper(dialog,"sg_inner_template");
	sg_inner_setters = response_configure_string_helper(dialog,"sg_inner_setters");
	sg_inner_getters = response_configure_string_helper(dialog,"sg_inner_getters");

	sg_outter_forward_template = response_configure_string_helper(dialog,"sg_outter_forward_template");
	sg_outter_forward_setters = response_configure_string_helper(dialog,"sg_outter_forward_setters");
	sg_outter_forward_getters = response_configure_string_helper(dialog,"sg_outter_forward_getters");
	sg_outter_template = response_configure_string_helper(dialog,"sg_outter_template");
	sg_outter_setters = response_configure_string_helper(dialog,"sg_outter_setters");
	sg_outter_getters = response_configure_string_helper(dialog,"sg_outter_getters");



	stash_group_save_to_file(group, config_filename, G_KEY_FILE_NONE);
}

void set_helper_toggle_button(GtkDialog *dialog, GtkWidget *vbox, gchar *descr, gchar *id, gboolean value) {
	GtkWidget *widget;
	
	widget = gtk_check_button_new_with_label(descr);
	gtk_toggle_button_set_active((GtkToggleButton *)widget,value);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 3);
	g_object_set_data(G_OBJECT(dialog), id, widget);	
}

void set_helper_edit_field(GtkDialog *dialog, GtkWidget *vbox, gboolean in, gchar *descr, gchar *id, gchar *value) {
	GtkWidget *frame, *view;
	GtkTextBuffer *buffer;
	
	frame = gtk_frame_new(descr);
	buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(buffer,value,strlen(value));
	view = gtk_text_view_new_with_buffer(buffer);
	gtk_container_add(GTK_CONTAINER(frame), view);
	if (in) {
		gtk_container_add(GTK_CONTAINER(vbox), frame);
	} else {
		gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 3);
	}
	g_object_set_data(G_OBJECT(dialog), id, view);	
}

GtkWidget* plugin_configure (GtkDialog *dialog)  {
	GtkWidget *global_vbox, *notebook;
	
	GtkWidget *tab1_vbox, *tab1_label;
	
	GtkWidget *tab2_vbox, *tab2_label, *tab2_hbox, *tab2_label1;

	GtkWidget *tab3_vbox, *tab3_label, *tab3_hbox, *tab3_label1;
	
	GtkWidget *tab4_vbox, *tab4_label, *tab4_hbox, *tab4_label1;
	
	GtkWidget *tab5_vbox, *tab5_label, *tab5_hbox, *tab5_label1;
	
	global_vbox = gtk_vbox_new(FALSE,10);
	
	tab1_vbox = gtk_vbox_new(FALSE,10);
	tab1_label = gtk_label_new_with_mnemonic("Main settings");
	set_helper_toggle_button(dialog, tab1_vbox,"Show interaction dialog","sg_show_interaction",sg_show_interaction);
	set_helper_toggle_button(dialog, tab1_vbox,"Place methods inside class by default","sg_placement_inner",sg_placement_inner);
	set_helper_toggle_button(dialog, tab1_vbox,"Create setter methods by default","sg_do_setters",sg_do_setters);
	set_helper_toggle_button(dialog, tab1_vbox,"Create getter methods by default","sg_do_getters",sg_do_getters);
	
	tab2_vbox = gtk_vbox_new(FALSE,10);
	tab2_label = gtk_label_new_with_mnemonic("Global templates");
	tab2_hbox = gtk_hbox_new(FALSE,10);
	set_helper_edit_field(dialog, tab2_hbox, TRUE, "Getter method name", "sg_method_name_getter", sg_method_name_getter);
	set_helper_edit_field(dialog, tab2_hbox, TRUE, "Setter method name", "sg_method_name_setter", sg_method_name_setter);
	gtk_box_pack_start(GTK_BOX(tab2_vbox), tab2_hbox, FALSE, FALSE, 3);
	tab2_label1 = gtk_label_new_with_mnemonic("$NAME - property name");
	gtk_box_pack_start(GTK_BOX(tab2_vbox), tab2_label1, FALSE, FALSE, 3);
	
	
	tab3_vbox = gtk_vbox_new(FALSE,10);
	tab3_label = gtk_label_new_with_mnemonic("Inner templates");
	tab3_hbox = gtk_hbox_new(FALSE,10);
	set_helper_edit_field(dialog, tab3_hbox, TRUE, "Getter method template", "sg_inner_getters", sg_inner_getters);
	set_helper_edit_field(dialog, tab3_hbox, TRUE, "Setter method template", "sg_inner_setters", sg_inner_setters);
	gtk_box_pack_start(GTK_BOX(tab3_vbox), tab3_hbox, FALSE, FALSE, 3);
	set_helper_edit_field(dialog, tab3_vbox, FALSE, "Master template", "sg_inner_template", sg_inner_template);
	tab3_label1 = gtk_label_new_with_mnemonic("$NAME - property name; $TYPE - property type\n$KIND - property kind; $CLASS - property class\n$METHNAME - method name from global templates");
	gtk_box_pack_start(GTK_BOX(tab3_vbox), tab3_label1, FALSE, FALSE, 3);
	
	tab4_vbox = gtk_vbox_new(FALSE,10);
	tab4_label = gtk_label_new_with_mnemonic("Outter declaration");
	tab4_hbox = gtk_hbox_new(FALSE,10);
	set_helper_edit_field(dialog, tab4_hbox, TRUE, "Forward getter method template", "sg_outter_forward_getters", sg_outter_forward_getters);
	set_helper_edit_field(dialog, tab4_hbox, TRUE, "Forward setter method template", "sg_outter_forward_setters", sg_outter_forward_setters);
	gtk_box_pack_start(GTK_BOX(tab4_vbox), tab4_hbox, FALSE, FALSE, 3);
	set_helper_edit_field(dialog, tab4_vbox, FALSE, "Forward master template", "sg_outter_forward_template", sg_outter_forward_template);
	tab4_label1 = gtk_label_new_with_mnemonic("$NAME - property name; $TYPE - property type\n$KIND - property kind; $CLASS - property class\n$METHNAME - method name from global templates");
	gtk_box_pack_start(GTK_BOX(tab4_vbox), tab4_label1, FALSE, FALSE, 3);
	
	tab5_vbox = gtk_vbox_new(FALSE,10);
	tab5_label = gtk_label_new_with_mnemonic("Outter definition");
	tab5_hbox = gtk_hbox_new(FALSE,10);
	set_helper_edit_field(dialog, tab5_hbox, TRUE, "Outter getter method template", "sg_outter_getters", sg_outter_getters);
	set_helper_edit_field(dialog, tab5_hbox, TRUE, "Outter setter method template", "sg_outter_setters", sg_outter_setters);
	gtk_box_pack_start(GTK_BOX(tab5_vbox), tab5_hbox, FALSE, FALSE, 3);
	set_helper_edit_field(dialog, tab5_vbox, FALSE, "Outter master template", "sg_outter_template", sg_outter_template);
	tab5_label1 = gtk_label_new_with_mnemonic("$NAME - property name; $TYPE - property type\n$KIND - property kind; $CLASS - property class\n$METHNAME - method name from global templates");
	gtk_box_pack_start(GTK_BOX(tab5_vbox), tab5_label1, FALSE, FALSE, 3);
	
	notebook = gtk_notebook_new();
	gtk_notebook_append_page((GtkNotebook *)notebook,tab1_vbox,tab1_label);
	gtk_notebook_append_page((GtkNotebook *)notebook,tab2_vbox,tab2_label);
	gtk_notebook_append_page((GtkNotebook *)notebook,tab3_vbox,tab3_label);
	gtk_notebook_append_page((GtkNotebook *)notebook,tab4_vbox,tab4_label);
	gtk_notebook_append_page((GtkNotebook *)notebook,tab5_vbox,tab5_label);
	
	gtk_container_add(GTK_CONTAINER(global_vbox), notebook);
	
	g_signal_connect(dialog, "response", G_CALLBACK(response_configure), NULL);
	
	gtk_widget_show_all(global_vbox);
	return global_vbox;
}

void plugin_cleanup(void) {
    gtk_widget_destroy(main_menu_item);
	free(config_filename);
	free(config_dirname);
}
