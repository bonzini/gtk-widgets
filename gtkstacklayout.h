/* GtkStackLayout: Widget for geometry management of arbitrary-sized areas.
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

#ifndef __GTK_STACK_LAYOUT_H__
#define __GTK_STACK_LAYOUT_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "gtklayoutmanager.h"


G_BEGIN_DECLS

#define GTK_TYPE_STACK_LAYOUT            (gtk_stack_layout_get_type ())
#define GTK_STACK_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_STACK_LAYOUT, GtkStackLayout))
#define GTK_STACK_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_STACK_LAYOUT, GtkStackLayoutClass))
#define GTK_IS_STACK_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_STACK_LAYOUT))
#define GTK_IS_STACK_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_STACK_LAYOUT))
#define GTK_STACK_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_STACK_LAYOUT, GtkStackLayoutClass))


#if 0
typedef struct _GtkStackLayout        GtkStackLayout;
typedef struct _GtkStackLayoutClass   GtkStackLayoutClass;
#endif

struct _GtkStackLayout
{
  GtkContainer container;

  GtkLayoutManager *root;
  GSList	   *current;

  gint height;
  gint width;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  /*< public >*/
  GdkWindow *bin_window;
};

struct _GtkStackLayoutClass
{
  GtkContainerClass parent_class;

  void  (*set_scroll_adjustments)   (GtkStackLayout *stack_layout,
				     GtkAdjustment  *hadjustment,
				     GtkAdjustment  *vadjustment);
};

GType          gtk_stack_layout_get_type        (void) G_GNUC_CONST;
GtkWidget*     gtk_stack_layout_new             (GtkAdjustment *hadjustment,
					         GtkAdjustment *vadjustment);
  
void	       gtk_stack_layout_push		(GtkStackLayout   *stack_layout,
			                         GtkLayoutManager *manager);
void	       gtk_stack_layout_pop		(GtkStackLayout   *stack_layout);

GtkAdjustment* gtk_stack_layout_get_hadjustment (GtkStackLayout     *stack_layout);
GtkAdjustment* gtk_stack_layout_get_vadjustment (GtkStackLayout     *stack_layout);
void           gtk_stack_layout_set_hadjustment (GtkStackLayout     *stack_layout,
						 GtkAdjustment *adjustment);
void           gtk_stack_layout_set_vadjustment (GtkStackLayout     *stack_layout,
						 GtkAdjustment *adjustment);


/* Private.  */
void	       gtk_stack_layout_allocate_child (GtkWidget       *widget,
						GdkPoint	*origin,
						GtkRequisition	*requisition);

G_END_DECLS

#endif /* __GTK_STACK_LAYOUT_H__ */
