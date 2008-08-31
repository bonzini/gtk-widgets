/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_LAYOUT_ADAPTOR_H__
#define __GTK_LAYOUT_ADAPTOR_H__


#include "gtklayoutmanager.h"


G_BEGIN_DECLS

#define GTK_TYPE_LAYOUT_ADAPTOR                  (gtk_layout_adaptor_get_type ())
#define GTK_LAYOUT_ADAPTOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_LAYOUT_ADAPTOR, GtkLayoutAdaptor))
#define GTK_LAYOUT_ADAPTOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_LAYOUT_ADAPTOR, GtkLayoutAdaptorClass))
#define GTK_IS_LAYOUT_ADAPTOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_LAYOUT_ADAPTOR))
#define GTK_IS_LAYOUT_ADAPTOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_LAYOUT_ADAPTOR))
#define GTK_LAYOUT_ADAPTOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_LAYOUT_ADAPTOR, GtkLayoutAdaptorClass))


typedef struct _GtkLayoutAdaptor       GtkLayoutAdaptor;
typedef struct _GtkLayoutAdaptorClass  GtkLayoutAdaptorClass;

struct _GtkLayoutAdaptor
{
  GtkLayoutManager layout_manager;

  GtkWidget *child;
};

struct _GtkLayoutAdaptorClass
{
  GtkLayoutManagerClass parent_class;
};


GType      gtk_layout_adaptor_get_type  (void) G_GNUC_CONST;

GtkLayoutManager *gtk_layout_adaptor_new (void);

GtkWidget *gtk_layout_adaptor_get_child (GtkLayoutAdaptor *layout_adaptor);
void gtk_layout_adaptor_set_child (GtkLayoutAdaptor *layout_adaptor,
				   GtkWidget	    *child);

G_END_DECLS

#endif /* __GTK_LAYOUT_ADAPTOR_H__ */
