/* GTK - The GIMP Toolkit
 * Copyright (C) 2008 Free Software Foundation, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_LAYOUTABLE_H__
#define __GTK_LAYOUTABLE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_LAYOUTABLE            (gtk_layoutable_get_type ())
#define GTK_LAYOUTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_LAYOUTABLE, GtkLayoutable))
#define GTK_LAYOUTABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), GTK_TYPE_LAYOUTABLE, GtkLayoutableIface))
#define GTK_IS_LAYOUTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_LAYOUTABLE))
#define GTK_LAYOUTABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GTK_TYPE_LAYOUTABLE, GtkLayoutableIface))

typedef struct _GtkLayoutable      GtkLayoutable; /* Dummy typedef */
typedef struct _GtkLayoutableIface GtkLayoutableIface;

struct _GtkLayoutableIface
{
  GTypeInterface g_iface;

  /* virtual table */
  void      (*size_request)    (GtkLayoutable        *layoutable,
				GtkRequisition       *requisition);
  void      (*size_allocate)   (GtkLayoutable        *layoutable,
				GtkAllocation        *allocation);
};


GType     gtk_layoutable_get_type               (void) G_GNUC_CONST;
void	  gtk_layoutable_init			(void);

void      gtk_layoutable_size_request          (GtkLayoutable        *layoutable,
						GtkRequisition       *requisition);
void      gtk_layoutable_size_allocate         (GtkLayoutable        *layoutable,
						GtkAllocation        *allocation);

G_END_DECLS

#endif /* __GTK_LAYOUTABLE_H__ */
