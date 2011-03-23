all:
	LANG=C gcc -c setters_getters.c -fPIC `llvm-config --cflags` `pkg-config --cflags geany` && gcc setters_getters.o -o setters_getters.so -shared `pkg-config --libs geany` -lclang  && sudo cp setters_getters.so /usr/lib/geany/ && geany -vi /tmp/lol/fff.cpp
