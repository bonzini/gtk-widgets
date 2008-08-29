CFLAGS = `pkg-config --cflags gtk+-2.0` -Wall -Wextra -g
LDFLAGS = `pkg-config --libs gtk+-2.0`

demo: gtkellipsis.o gtkresizer.o gtkresizermarshal.o demo.o

gtkellipsis.o: gtkellipsis.c gtkellipsis.h
gtkresizer.o: gtkresizer.c gtkresizermarshal.h gtkresizer.h
gtkresizermarshal.o: gtkresizermarshal.c gtkresizermarshal.h
demo.o: demo.c gtkresizer.h gtkellipsis.h

%marshal.c: %marshal.in
	glib-genmarshal --prefix=$(*:gtk%=gtk_%)_marshal --body $< > $@

%marshal.h: %marshal.in
	glib-genmarshal --prefix=$(*:gtk%=gtk_%)_marshal --header $< > $@
