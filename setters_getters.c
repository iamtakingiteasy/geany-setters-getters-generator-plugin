#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "geanyplugin.h"
#include <clang-c/Index.h>

GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;

PLUGIN_VERSION_CHECK(147)

PLUGIN_SET_INFO("C++ setters/getters generator", "Generates C++ setters and getters methods",
                "0.01-alpha", "Alexander Tumin <itakingiteasy@gmail.com>");

enum PropertyKind {
	INVALID_KIND,
	PUBLIC,
	PRIVATE,
	PROTECTED
};

struct Property {
	char *type;
	char *name;
	enum PropertyKind kind;
};

struct PropertyList {
		struct Property *data;
		int allocated;
		int used;
};


void init_proplist(struct PropertyList *proplist) {
	proplist->allocated = 0;
	proplist->used = 0;
	proplist->data = NULL;
}

void add_prop(struct PropertyList *proplist, char *type, int typelen, char *name, int namelen, enum PropertyKind kind) {
	proplist->used++;
	if (proplist->used > proplist->allocated) {
		proplist->allocated++;
		proplist->data = realloc(proplist->data, proplist->allocated * sizeof (struct Property));
		proplist->data[proplist->allocated-1].type = malloc((typelen +1) * sizeof(char));  /* +1 is for */
		proplist->data[proplist->allocated-1].name = malloc((namelen + 1) * sizeof(char)); /* null terminator */
		proplist->data[proplist->allocated-1].kind = INVALID_KIND;
	}
	strncpy(proplist->data[proplist->allocated-1].type,type,typelen);
	strncpy(proplist->data[proplist->allocated-1].name,name,namelen);
	proplist->data[proplist->allocated-1].kind = kind;
}

void free_proplist(struct PropertyList *proplist) {
	int i;
	for (i=0; i < proplist->allocated; i++) {
		free(proplist->data[i].type);
		free(proplist->data[i].name);
		proplist->data[i].type = NULL;
		proplist->data[i].name = NULL;
		proplist->data[i].kind = INVALID_KIND;
	}
	
	free(proplist->data);
	
	proplist->allocated = 0;
	proplist->used = 0;
	proplist->data = NULL;
};


static GtkWidget *main_menu_item = NULL;
enum PropertyKind propkind;
struct PropertyList proplist;

enum PropertyKind helper_getKind(ScintillaObject *sci,CXSourceLocation loc) {
	gchar *keyword;
	unsigned int offset;
	enum PropertyKind kind;
	
	clang_getInstantiationLocation(loc,NULL,NULL,NULL,&offset);
	keyword = sci_get_contents_range(sci,offset,offset+9);
	
	
	if (strncmp("protected",keyword,9) == 0) {
		kind = PROTECTED;
	} else if (strncmp("private",keyword,7) == 0) {
		kind = PRIVATE;
	} else if (strncmp("public",keyword,6) == 0) {
		kind = PUBLIC;
	}
	free(keyword);
	return kind;
}

int isvariable(char c) {
	if (isalnum(c) || c == '_')
		return 1;
	return 0;
}
int isedge(char c, char pc) {
	if (c == ';' || c == ':' || c == '{' || c == '}')
		return 1;
	if (c == '\n' && pc != '\\')
		return 1;
	return 0;
}

char *trim_left_string(char *str) {
	gint len = strlen(str);
	gint begining = 0;
	gchar *trimmed = NULL;
	while (isspace(str[begining++]));
	begining--;
	trimmed = malloc((len-begining+1) * sizeof(gchar));
	strncpy(trimmed,(str+begining),len-begining+1);
	
	return trimmed;
}

char *helper_getType(ScintillaObject *sci, CXSourceLocation loc) {
	gchar c,pv;
	unsigned int offset;
	gchar *type_string = NULL;
	size_t t_beg = 0;
	size_t t_end = 0;

	clang_getInstantiationLocation(loc,NULL,NULL,NULL,&offset);
	int is_type_begining = FALSE;
	int is_edge = FALSE;
	int is_suceess = FALSE;
	
	while (is_type_begining == FALSE && is_edge == FALSE) {
		while (isspace((c = sci_get_char_at(sci,--offset))));
		if (c == ',') {
			while (isspace((c = sci_get_char_at(sci,--offset))));
			while (!isspace((c = sci_get_char_at(sci,--offset))));
			continue;
		}

		pv = sci_get_char_at(sci,offset-1);		
		if (isedge(c,pv)) {
			if (type_string == NULL) {
				if (is_suceess == FALSE) {
					type_string = malloc(4 * sizeof(gchar));
					strncpy(type_string,"int",4);
				} else {
					t_beg = offset + 1;
					gchar *untrimmed_type = sci_get_contents_range(sci,t_beg,t_end);
					type_string = trim_left_string(untrimmed_type);
					free(untrimmed_type);
				}
				is_edge = TRUE;
			}
		}
		if (is_suceess == FALSE) {
			t_end = offset + 1;
			is_suceess = TRUE;	
		}
	}
	//printf("%c\n",sci_get_char_at(sci,offset));
	return type_string;
}

enum CXChildVisitResult traverser(CXCursor cursor, CXCursor parent, CXClientData client_data) {
	ScintillaObject *sci = (ScintillaObject*)client_data;
	enum CXCursorKind curkind;
	CXSourceLocation loc;
	
	loc = clang_getCursorLocation(cursor);
	curkind = clang_getCursorKind(cursor);
	/* is this supposed to be 'private:' or 'public:' keywords ? */
	if (curkind == 1) {
		propkind = helper_getKind(sci,loc);
	}
	/* class property */
	if (curkind == 6) {
		gchar *type = NULL;
		gchar *name  = NULL;
		type = helper_getType(sci,loc);
		name = (gchar *)clang_getCString(clang_getCursorSpelling(cursor));
		add_prop(&proplist,type,strlen(type)+1,name,strlen(name)+1,propkind);
		free(type);
		free(name);
	}
	return CXChildVisit_Continue;
}

struct ChunkedString {
	gchar *data;
	size_t allocated;
	size_t used;
	size_t chunk;
};

void add_chunked_string(struct ChunkedString *chstr, gchar *string) {
	int len = strlen(string);
	chstr->used += len;
	while (chstr->used > chstr->allocated) {
			chstr->allocated += chstr->chunk;
			chstr->data = realloc(chstr->data, chstr->allocated * sizeof(gchar));
	}
	strncpy((chstr->data+chstr->used-len),string,len);
}

gchar *gen_setters_getters(struct PropertyList *proplist) {
	int i;
	struct ChunkedString result;
	result.data = NULL;
	result.allocated = 0;
	result.used = 0;
	result.chunk = 1024;
	
	add_chunked_string(&result,"\n\tpublic:\n");
	for (i=0; i < proplist->used; i++) {
		add_chunked_string(&result, "\t\tvoid set_");
		add_chunked_string(&result, proplist->data[i].name);
		add_chunked_string(&result, "(");
		add_chunked_string(&result, proplist->data[i].type);
		add_chunked_string(&result, " value) {\n\t\t\tthis->");
		add_chunked_string(&result, proplist->data[i].name);
		add_chunked_string(&result, " = value;\n\t\t}\n");
	}
	
	add_chunked_string(&result,"\n\tpublic:\n");
	for (i=0; i < proplist->used; i++) {
		add_chunked_string(&result, "\t\t");
		add_chunked_string(&result, proplist->data[i].type);
		add_chunked_string(&result, " get_");
		add_chunked_string(&result, proplist->data[i].name);
		add_chunked_string(&result, "(void) {\n\t\t\treturn tthis->");
		add_chunked_string(&result, proplist->data[i].name);
		add_chunked_string(&result, ";\n\t\t}\n");
	}
	
	printf("RR> %s\n",result.data);
	return result.data;
}

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata) {
	GeanyDocument *doc = NULL;
	ScintillaObject *sci = NULL;
	char *source;
	gchar *keyword;
	unsigned int offset;
	unsigned int insert;
	gchar *settersgetters = NULL;
	
	CXIndex Index;
	CXTranslationUnit TU;
	CXSourceLocation source_loc;
	CXFile file;
	struct CXUnsavedFile USF;
	CXCursor cur;
	enum CXCursorKind curkind;
	CXSourceLocation loc;
	
	size_t length = 0;
	gint line = 0;
	gint position = 0;
	gint column = 0;
	
	/******************************************************************/
	
	doc = document_get_current();
	sci = doc->editor->sci;
	
	length = sci_get_length(sci)+1;
	line = sci_get_current_line(sci)+1;
	position = sci_get_current_position(sci);
	column = sci_get_col_from_position(sci,position)+1;
	
	source = sci_get_contents(sci,length);
	
	init_proplist(&proplist);
	
	Index = clang_createIndex(0,0);
	USF.Filename = "NonExistingFile.cpp";
	USF.Contents = source;
	USF.Length = length;
	
	TU = clang_parseTranslationUnit(Index,"NonExistingFile.cpp",NULL,0,&USF,1,CXTranslationUnit_Incomplete);
	file = clang_getFile(TU,"NonExistingFile.cpp");
	source_loc = clang_getLocation(TU,file,line,column);
	
	cur = clang_getCursor(TU,source_loc);
	curkind = clang_getCursorKind(cur);
	
	while (curkind != 70 && curkind != 4) {
		cur = clang_getCursorLexicalParent(cur);
		curkind = clang_getCursorKind(cur);
	}
	
	if (curkind == 70) {
		dialogs_show_msgbox(GTK_MESSAGE_INFO, "Not inside the class. Aborting.");
		return;
	}
	
	propkind = PRIVATE;

	clang_visitChildren(cur,traverser,sci);

	loc = clang_getCursorLocation(cur);
	clang_getInstantiationLocation(loc,NULL,NULL,NULL,&offset);
	/*
	 14:02   dgregor  Wooga: clang_getCursorExtent() gets the full source range of a cursor, 
                 as a begin/end pair
 
	 */
	while ((sci_get_char_at(sci,++offset)) != '{' );
	
	settersgetters = gen_setters_getters(&proplist);
	
	insert = sci_find_matching_brace(sci,offset);
	sci_insert_text(sci,insert,settersgetters);
	//dialogs_show_msgbox(GTK_MESSAGE_INFO, ">%d<",column);
	
	free(settersgetters);
	free(keyword);
	free_proplist(&proplist);
	
	free(source);
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
