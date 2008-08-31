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

#ifndef __GTK_FLOW_LAYOUT_MANAGER_H__
#define __GTK_FLOW_LAYOUT_MANAGER_H__

#include <glib-2.0/glib.h>
#include "gtklayoutmanager.h"


G_BEGIN_DECLS

#define GTK_TYPE_FLOW_LAYOUT_MANAGER                  (gtk_flow_layout_manager_get_type ())
#define GTK_FLOW_LAYOUT_MANAGER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FLOW_LAYOUT_MANAGER, GtkFlowLayoutManager))
#define GTK_FLOW_LAYOUT_MANAGER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FLOW_LAYOUT_MANAGER, GtkFlowLayoutManagerClass))
#define GTK_IS_FLOW_LAYOUT_MANAGER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FLOW_LAYOUT_MANAGER))
#define GTK_IS_FLOW_LAYOUT_MANAGER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FLOW_LAYOUT_MANAGER))
#define GTK_FLOW_LAYOUT_MANAGER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FLOW_LAYOUT_MANAGER, GtkFlowLayoutManagerClass))


typedef struct _GtkFlowLayoutManager       GtkFlowLayoutManager;
typedef struct _GtkFlowLayoutManagerClass  GtkFlowLayoutManagerClass;

struct _GtkFlowLayoutManager
{
  GtkLayoutManager layout_manager;

  GList *children;
  GList **p_tail;
};

struct _GtkFlowLayoutManagerClass
{
  GtkLayoutManagerClass parent_class;
};


GType             gtk_flow_layout_manager_get_type  (void) G_GNUC_CONST;

GtkLayoutManager *gtk_flow_layout_manager_new (void);

G_END_DECLS

#endif /* __GTK_FLOW_LAYOUT_MANAGER_H__ */
