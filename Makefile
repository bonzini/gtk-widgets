CFLAGS = `pkg-config --cflags gtk+-2.0` -Wall -Wextra -g
LDFLAGS = `pkg-config --libs gtk+-2.0`

WIDGETS = gtkellipsis.o gtkresizer.o gtkresizermarshal.o \
	gtkmanagedlayout.o gtklayoutmanager.o gtklayoutadaptor.o \
	gtkmanagedlayoutmarshal.o gtkstacklayoutmanager.o \
	gtkflowlayoutmanager.o

all: demo layout
demo: demo.o $(WIDGETS)
layout: layout.o $(WIDGETS)

gtkellipsis.o: gtkellipsis.c gtkellipsis.h
gtkresizer.o: gtkresizer.c gtkresizermarshal.h gtkresizer.h
gtkresizermarshal.o: gtkresizermarshal.c gtkresizermarshal.h
demo.o: demo.c gtkresizer.h gtkellipsis.h

gtkmanagedlayout.o: gtkmanagedlayout.c gtkmanagedlayoutmarshal.h gtkmanagedlayout.h
gtklayoutmanager.o: gtklayoutmanager.c gtkmanagedlayoutmarshal.h gtklayoutmanager.h gtkmanagedlayout.h
gtkstacklayoutmanager.o: gtkstacklayoutmanager.c gtkstacklayoutmanager.h
gtkmanagedlayoutmarshal.o: gtkmanagedlayoutmarshal.c gtkmanagedlayoutmarshal.h
gtklayoutadaptor.o: gtklayoutadaptor.c gtklayoutadaptor.h

%marshal.c: %marshal.in
	glib-genmarshal --prefix=$(*:gtk%=gtk_%)_marshal --body $< > $@

%marshal.h: %marshal.in
	glib-genmarshal --prefix=$(*:gtk%=gtk_%)_marshal --header $< > $@
