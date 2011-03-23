FLAGS = -g -shared `pkg-config --libs geany` -lclang
CFLAGS = -pedantic -Wall -O3 -g `llvm-config --cflags` `pkg-config --cflags geany`

PLUGIN_NAME = setters_getters.so

all: $(PLUGIN_NAME)

clean:
	rm -f setters_getters.so setters_getters.o chunked_strings_crutch.o chunked_clang_property_crutch.o
	
install:
	cp $(PLUGIN_NAME) /usr/lib/geany/

develop_run:
	geany -vi /tmp/lol/fff.cpp

setters_getters.o: setters_getters.h setters_getters.c
chunked_strings_crutch.o: chunked_strings_crutch.c chunked_strings_crutch.h
chunked_clang_property_crutch.o: chunked_clang_property_crutch.c chunked_clang_property_crutch.h

$(PLUGIN_NAME): setters_getters.o chunked_strings_crutch.o chunked_clang_property_crutch.o
	$(CC) -o $(PLUGIN_NAME) $(FLAGS) setters_getters.o chunked_strings_crutch.o chunked_clang_property_crutch.o
