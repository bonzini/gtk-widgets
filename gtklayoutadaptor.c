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

#include <gtk/gtk.h>
#include "gtkmanagedlayout.h"
#include "gtklayoutmanager.h"
#include "gtklayoutadaptor.h"

enum {
  PROP_0,
  PROP_CHILD
};  

#define I_(x)		(x)
#define P_(x)		(x)

static void gtk_layout_adaptor_finalize (GObject *object);
static void gtk_layout_adaptor_set_property (GObject         *object,
			                     guint            prop_id,
			                     const GValue    *value,
			                     GParamSpec      *pspec);
static void gtk_layout_adaptor_get_property (GObject         *object,
		                             guint            prop_id,
		                             GValue          *value,
		                             GParamSpec      *pspec);
static void gtk_layout_adaptor_foreach_widget      (GtkLayoutManager   *layout_manager,
						 GtkCallback     callback,
						 gpointer        callback_data);
static void gtk_layout_adaptor_get_requisition (GtkLayoutManager *layout_manager,
		                                GtkRequisition *requisition);
static void gtk_layout_adaptor_size_request (GtkLayoutManager *layout_manager,
		                             GtkRequisition *requisition);
static void gtk_layout_adaptor_size_allocate (GtkLayoutManager *layout_manager,
	                                      GtkAllocation    *allocation);

G_DEFINE_TYPE (GtkLayoutAdaptor, gtk_layout_adaptor, GTK_TYPE_LAYOUT_MANAGER)

static void
gtk_layout_adaptor_class_init (GtkLayoutAdaptorClass *class)
{
  GObjectClass *gobject_class;
  GtkLayoutManagerClass *layout_manager_class;

  gobject_class = G_OBJECT_CLASS (class);
  layout_manager_class = (GtkLayoutManagerClass*) class;

  gobject_class->set_property = gtk_layout_adaptor_set_property;
  gobject_class->get_property = gtk_layout_adaptor_get_property;
  gobject_class->finalize = gtk_layout_adaptor_finalize;

  layout_manager_class->foreach_widget = gtk_layout_adaptor_foreach_widget;
  layout_manager_class->get_requisition = gtk_layout_adaptor_get_requisition;
  layout_manager_class->size_request = gtk_layout_adaptor_size_request;
  layout_manager_class->size_allocate = gtk_layout_adaptor_size_allocate;

  g_object_class_install_property (gobject_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child",
                                                      P_("Child"),
                                                      P_("The child widget drawn in the layout adaptor"),
                                                      GTK_TYPE_WIDGET,
                                                      G_PARAM_READWRITE));
}

static void
gtk_layout_adaptor_init (GtkLayoutAdaptor *layout_adaptor)
{
  layout_adaptor->child = NULL;
}

static void
gtk_layout_adaptor_finalize (GObject *object)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (object);

  if (layout_adaptor->child)
    g_object_unref (layout_adaptor->child);

  (* G_OBJECT_CLASS (gtk_layout_adaptor_parent_class)->finalize) (object);
}



static void
gtk_layout_adaptor_get_property (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      g_value_set_object (value, layout_adaptor->child);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_layout_adaptor_set_property (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (object);

  switch (prop_id)
    {
    case PROP_CHILD:
      gtk_layout_adaptor_set_child (layout_adaptor, GTK_WIDGET (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GtkLayoutManager *
gtk_layout_adaptor_new (void)
{
  return g_object_new (GTK_TYPE_LAYOUT_ADAPTOR, NULL);
}

GtkWidget *
gtk_layout_adaptor_get_child (GtkLayoutAdaptor *layout_adaptor)
{
  return layout_adaptor->child;
}

void
gtk_layout_adaptor_set_child (GtkLayoutAdaptor *layout_adaptor,
			      GtkWidget    *child)
{
  GtkLayoutManager *layout_manager = GTK_LAYOUT_MANAGER (layout_adaptor);

  g_return_if_fail (GTK_IS_WIDGET (child));

  if (child)
    {
      GtkManagedLayout *root = GTK_LAYOUT_MANAGER (layout_adaptor)->root;
      gtk_widget_set_parent (child, GTK_WIDGET (root));
      layout_adaptor->child = child;
    }
  else
    {
      gboolean widget_was_visible = GTK_WIDGET_VISIBLE (layout_adaptor->child);
      gtk_widget_unparent (layout_adaptor->child);
      layout_adaptor->child = NULL;
  
      if (widget_was_visible)
        gtk_layout_manager_queue_resize (layout_manager);
    }
}

static void
gtk_layout_adaptor_foreach_widget (GtkLayoutManager *layout_manager,
				GtkCallback   callback,
				gpointer      callback_data)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (layout_manager);

  g_return_if_fail (callback != NULL);

  if (layout_adaptor->child)
    (* callback) (layout_adaptor->child, callback_data);
}

static void
gtk_layout_adaptor_size_request (GtkLayoutManager *layout_manager,
                                 GtkRequisition *requisition)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (layout_manager);

  if (layout_adaptor->child)
    {
      GtkRequisition child_requisition;

      gtk_widget_size_request (layout_adaptor->child, &child_requisition);
      requisition->width = child_requisition.width + 2 * layout_manager->border_width;
      requisition->height = child_requisition.height + 2 * layout_manager->border_width;
    }
  else
    requisition->width = requisition->height = 0;

  layout_manager->requisition = *requisition;
}

static void
gtk_layout_adaptor_get_requisition (GtkLayoutManager *layout_manager,
                                    GtkRequisition *requisition)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (layout_manager);
  GtkWidget *widget = layout_adaptor->child;

  gtk_widget_get_child_requisition (widget, requisition);
}

static void
gtk_layout_adaptor_size_allocate (GtkLayoutManager *layout_manager,
                                  GtkAllocation    *allocation)
{
  GtkLayoutAdaptor *layout_adaptor = GTK_LAYOUT_ADAPTOR (layout_manager);
  GtkWidget *widget = layout_adaptor->child;

  if (widget)
    {
      GtkRequisition child_requisition;
      GtkAllocation child_allocation;
      int border_width;

      gtk_layout_manager_get_requisition (layout_manager, &child_requisition);

      border_width = layout_manager->border_width;
      child_allocation.x = allocation->x + border_width;
      child_allocation.y = allocation->y + border_width;

      /* A hack.  This will go away if our layout model becomes an interface
	 that GtkLabel can implement.  */
      if (GTK_IS_LABEL (widget)
	  && gtk_label_get_line_wrap (GTK_LABEL (widget)))
	{
          PangoLayout *layout;
	  PangoRectangle rect;
          gfloat xalign, yalign;

          gtk_misc_get_alignment (&GTK_LABEL (widget)->misc, &xalign, &yalign);
          gtk_misc_set_alignment (&GTK_LABEL (widget)->misc, 0.0, yalign);
	  layout = gtk_label_get_layout (GTK_LABEL (widget));
          pango_layout_set_width (layout, allocation->width * PANGO_SCALE);
          pango_layout_get_extents (layout, NULL, &rect);

          child_allocation.width = rect.width / PANGO_SCALE;
          child_allocation.height = rect.height / PANGO_SCALE;
	}
      else
	{
	  child_allocation.width = child_requisition.width;
	  child_allocation.height = child_requisition.height;
	}

      allocation->width = child_allocation.width + 2 * border_width;
      allocation->height = child_allocation.height + 2 * border_width;
      gtk_widget_size_allocate (layout_adaptor->child, &child_allocation);
    }
}
