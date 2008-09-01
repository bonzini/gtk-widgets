/* GtkManagedLayout: Widget for geometry management of arbitrary-sized areas.
 * Copyright (C) 2008 Free Software Foundation, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __GTK_MANAGED_LAYOUT_H__
#define __GTK_MANAGED_LAYOUT_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_MANAGED_LAYOUT            (gtk_managed_layout_get_type ())
#define GTK_MANAGED_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_MANAGED_LAYOUT, GtkManagedLayout))
#define GTK_MANAGED_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_MANAGED_LAYOUT, GtkManagedLayoutClass))
#define GTK_IS_MANAGED_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_MANAGED_LAYOUT))
#define GTK_IS_MANAGED_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MANAGED_LAYOUT))
#define GTK_MANAGED_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_MANAGED_LAYOUT, GtkManagedLayoutClass))

typedef struct _GtkManagedLayout        GtkManagedLayout;
typedef struct _GtkManagedLayoutClass   GtkManagedLayoutClass;

struct _GtkManagedLayout
{
  GtkBin bin;

  gint height;
  gint width;
  gint requested_height;
  gint requested_width;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  /*< public >*/
  GdkWindow *bin_window;
};

struct _GtkManagedLayoutClass
{
  GtkContainerClass parent_class;

  void  (*set_scroll_adjustments)   (GtkManagedLayout *managed_layout,
				     GtkAdjustment  *hadjustment,
				     GtkAdjustment  *vadjustment);
};

GType          gtk_managed_layout_get_type        (void) G_GNUC_CONST;
GtkWidget*     gtk_managed_layout_new             (GtkAdjustment *hadjustment,
					         GtkAdjustment *vadjustment);
  
GtkAdjustment* gtk_managed_layout_get_hadjustment (GtkManagedLayout     *managed_layout);
GtkAdjustment* gtk_managed_layout_get_vadjustment (GtkManagedLayout     *managed_layout);
void           gtk_managed_layout_set_hadjustment (GtkManagedLayout     *managed_layout,
						 GtkAdjustment *adjustment);
void           gtk_managed_layout_set_vadjustment (GtkManagedLayout     *managed_layout,
						 GtkAdjustment *adjustment);


G_END_DECLS

#endif /* __GTK_MANAGED_LAYOUT_H__ */
