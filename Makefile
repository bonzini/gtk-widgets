CFLAGS = `pkg-config --cflags gtk+-2.0` -Wall -Wextra -g
LDFLAGS = `pkg-config --libs gtk+-2.0`

WIDGETS = gtkellipsis.o gtkresizer.o gtkresizermarshal.o \
	gtklayoutable.o gtkmanagedlayout.o gtkmanagedlayoutmarshal.o

all: demo layout
demo: demo.o $(WIDGETS)
layout: layout.o $(WIDGETS)

gtkellipsis.o: gtkellipsis.c gtkellipsis.h
gtkresizer.o: gtkresizer.c gtkresizermarshal.h gtkresizer.h
gtkresizermarshal.o: gtkresizermarshal.c gtkresizermarshal.h
demo.o: demo.c gtkresizer.h gtkellipsis.h

gtklayoutable.o: gtklayoutable.c gtklayoutable.h
gtkmanagedlayout.o: gtkmanagedlayout.c gtkmanagedlayoutmarshal.h gtkmanagedlayout.h

%marshal.c: %marshal.in
	glib-genmarshal --prefix=$(*:gtk%=gtk_%)_marshal --body $< > $@

%marshal.h: %marshal.in
	glib-genmarshal --prefix=$(*:gtk%=gtk_%)_marshal --header $< > $@
