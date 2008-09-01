/* gtklayoutable.c
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


#include <gtk/gtk.h>
#include "gtklayoutable.h"

#define I_(x)		(x)
#define P_(x)		(x)

static void		gtk_widget_add_layoutable_interface ();
static void		gtk_label_add_layoutable_interface ();
static void		gtk_hbox_add_layoutable_interface ();
static void		gtk_vbox_add_layoutable_interface ();

GType
gtk_layoutable_get_type (void)
{
  static GType layoutable_type = 0;

  if (!layoutable_type)
    {
      layoutable_type =
        g_type_register_static_simple (G_TYPE_INTERFACE, I_("GtkLayoutable"),
				       sizeof (GtkLayoutableIface),
				       NULL, 0, NULL, 0);
    }

  return layoutable_type;
}

void
gtk_layoutable_init (void)
{
  static int once_only = 0;
  if (once_only)
    return;

  once_only = 1;
  gtk_widget_add_layoutable_interface ();
  gtk_label_add_layoutable_interface ();
  gtk_hbox_add_layoutable_interface ();
  gtk_vbox_add_layoutable_interface ();
}

/**
 * gtk_layoutable_size_request:
 * @layoutable: a #GtkLayoutable
 * @requisition: a #GtkRequisition
 **/
void
gtk_layoutable_size_request (GtkLayoutable        *layoutable,
                             GtkRequisition       *requisition)
{
  GtkLayoutableIface *iface;

  g_return_if_fail (GTK_IS_LAYOUTABLE (layoutable));
  g_return_if_fail (requisition != NULL);

  iface = GTK_LAYOUTABLE_GET_IFACE (layoutable);
  if (iface->size_request)
    (* iface->size_request) (layoutable, requisition);
}

void
gtk_layoutable_size_allocate (GtkLayoutable        *layoutable,
                              GtkAllocation        *allocation)
{
  GtkLayoutableIface *iface;

  g_return_if_fail (GTK_IS_LAYOUTABLE (layoutable));
  g_return_if_fail (allocation != NULL);
  g_return_if_fail (allocation->height == 0);

  iface = GTK_LAYOUTABLE_GET_IFACE (layoutable);
  if (iface->size_allocate)
    (* iface->size_allocate) (layoutable, allocation);
}


static void gtk_widget_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                                 GtkAllocation        *allocation);

static GtkLayoutableIface    *gtk_widget_parent_layoutable_iface;

static void
gtk_widget_layoutable_init (GtkLayoutableIface *iface)
{
  gtk_widget_parent_layoutable_iface = g_type_interface_peek_parent (iface);
  iface->size_request = (void (*) (GtkLayoutable *, GtkRequisition *)) gtk_widget_size_request;
  iface->size_allocate = gtk_widget_layoutable_size_allocate;
}

static void
gtk_widget_add_layoutable_interface ()
{
  static const GInterfaceInfo layoutable_info =
  {
    (GInterfaceInitFunc) gtk_widget_layoutable_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (gtk_widget_get_type (),
                               GTK_TYPE_LAYOUTABLE,
                               &layoutable_info);
}

static void
gtk_widget_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                     GtkAllocation        *allocation)
{
  GtkWidget *widget;
  GtkRequisition requisition;

  widget = GTK_WIDGET (layoutable);
  gtk_widget_get_child_requisition (widget, &requisition);
  allocation->width = requisition.width;
  allocation->height = requisition.height;
  gtk_widget_size_allocate (widget, allocation);
}

static void gtk_label_layoutable_size_request (GtkLayoutable        *layoutable,
		                               GtkRequisition       *requisition);
static void gtk_label_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                                GtkAllocation        *allocation);

static GtkLayoutableIface    *gtk_label_parent_layoutable_iface;

static void
gtk_label_layoutable_init (GtkLayoutableIface *iface)
{
  gtk_label_parent_layoutable_iface = g_type_interface_peek_parent (iface);
  iface->size_request = gtk_label_layoutable_size_request;
  iface->size_allocate = gtk_label_layoutable_size_allocate;
}

static void
gtk_label_add_layoutable_interface ()
{
  static const GInterfaceInfo layoutable_info =
  {
    (GInterfaceInitFunc) gtk_label_layoutable_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (gtk_label_get_type (),
                               GTK_TYPE_LAYOUTABLE,
                               &layoutable_info);
}

static void
gtk_label_layoutable_size_request (GtkLayoutable        *layoutable,
                                   GtkRequisition       *requisition)
{
  GtkLabel *label = GTK_LABEL (layoutable);

  if (gtk_label_get_line_wrap (label))
    requisition->width = requisition->height = 0;
  else
    gtk_widget_size_request (GTK_WIDGET (label), requisition);
}

static void
gtk_label_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                    GtkAllocation        *allocation)
{
  GtkLabel *label = GTK_LABEL (layoutable);

  if (gtk_label_get_line_wrap (label))
    {
      PangoLayout *layout;
      PangoRectangle rect;

      /* Make it span the entire line.  */
      layout = gtk_label_get_layout (label);
      pango_layout_set_width (layout, allocation->width * PANGO_SCALE);
      pango_layout_get_extents (layout, NULL, &rect);

      allocation->width = rect.width / PANGO_SCALE + label->misc.xpad * 2;
      allocation->height = rect.height / PANGO_SCALE + label->misc.ypad * 2;

      gtk_misc_set_alignment (&label->misc, 0.0, 0.0);
      gtk_widget_size_allocate (GTK_WIDGET (label), allocation);
    }

  else
    (gtk_label_parent_layoutable_iface->size_allocate) (layoutable, allocation);
}

static void gtk_box_children_size_request (GtkWidget *child,
                                           gpointer   client_data);
static void gtk_box_layoutable_size_request  (GtkLayoutable        *layoutable,
                                               GtkRequisition       *requisition);
static void gtk_hbox_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                               GtkAllocation        *allocation);

static GtkLayoutableIface    *gtk_hbox_parent_layoutable_iface;

static void
gtk_hbox_layoutable_init (GtkLayoutableIface *iface)
{
  gtk_hbox_parent_layoutable_iface = g_type_interface_peek_parent (iface);
  iface->size_request = gtk_box_layoutable_size_request;
  iface->size_allocate = gtk_hbox_layoutable_size_allocate;
}

static void
gtk_hbox_add_layoutable_interface ()
{
  static const GInterfaceInfo layoutable_info =
  {
    (GInterfaceInitFunc) gtk_hbox_layoutable_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (gtk_hbox_get_type (),
                               GTK_TYPE_LAYOUTABLE,
                               &layoutable_info);
}

static void
gtk_box_children_size_request (GtkWidget *child,
                               gpointer   client_data)
{
  GtkRequisition *requisition = client_data;
  GtkRequisition child_requisition = { 0, 0 };

  gtk_layoutable_size_request (GTK_LAYOUTABLE (child), &child_requisition);
  requisition->width = MAX (child_requisition.width, requisition->width);
  requisition->height = MAX (child_requisition.height, requisition->height);
}

static void
gtk_box_layoutable_size_request  (GtkLayoutable        *layoutable,
                                  GtkRequisition       *requisition)
{
  GtkContainer *container;
  GtkBox *box;

  container = GTK_CONTAINER (layoutable);
  box = GTK_BOX (container);

  if (box->homogeneous)
    {
      gtk_widget_size_request (GTK_WIDGET (box), requisition);
      return;
    }

  requisition->width = 0;
  requisition->height = 0;
  gtk_container_foreach (container,
                         gtk_box_children_size_request,
                         requisition);

  requisition->width += 2 * container->border_width;
  requisition->height += 2 * container->border_width;
}

static void
gtk_hbox_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                   GtkAllocation        *allocation)
{
  GtkBox *box = GTK_BOX (layoutable);
  guint pack = GTK_PACK_START;
  GList *list;
  gint i;
  gint border_width;
  gint row_height, row_width;
  gint available_width;
  GtkAllocation child_allocation;

  if (box->homogeneous)
    {
      (gtk_hbox_parent_layoutable_iface->size_allocate) (layoutable, allocation);
      return;
    }

  border_width = GTK_CONTAINER (box)->border_width;
  row_height = 0;
  row_width = border_width;
  available_width = allocation->width - border_width;

  allocation->width = 0;
  for (i = 0; i < 2; i++, pack = GTK_PACK_END)
    {
      list = pack == GTK_PACK_END ? g_list_last (box->children) : box->children;
      while (list)
        {
          GtkBoxChild *child_info = list->data;
          GtkLayoutable *child;
          GtkRequisition child_requisition;

          if (child_info->pack == pack
	      && GTK_WIDGET_VISIBLE (child_info->widget))
            {
	      child = GTK_LAYOUTABLE (child_info->widget);
	      gtk_widget_get_child_requisition (child_info->widget,
					        &child_requisition);

	      if (row_width + child_requisition.width > available_width)
	        {
	          /* Tell the parent about our actual allocation.  */
	          allocation->width = MAX (allocation->width, row_width + border_width);
	          allocation->height += row_height + box->spacing;
	          row_width = border_width;
	          row_height = 0;
	        }

	      child_allocation.x = allocation->x + row_width + child_info->padding;
	      child_allocation.y = allocation->y + allocation->height;
	      child_allocation.width = available_width;
	      child_allocation.height = 0;
	      gtk_layoutable_size_allocate (child, &child_allocation);

	      row_width += child_allocation.width + box->spacing +
		           child_info->padding * 2;
	      row_height = MAX (row_height, child_allocation.height);
            }

	  list = pack == GTK_PACK_END ? list->prev : list->next;
	}
    }

  allocation->width = MAX (allocation->width, row_width + border_width);
  if (row_height == 0)
    allocation->height += border_width - box->spacing;
  else
    allocation->height += row_height + border_width;
}

static void gtk_vbox_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                               GtkAllocation        *allocation);

static GtkLayoutableIface    *gtk_vbox_parent_layoutable_iface;

static void
gtk_vbox_layoutable_init (GtkLayoutableIface *iface)
{
  gtk_vbox_parent_layoutable_iface = g_type_interface_peek_parent (iface);
  iface->size_request = gtk_box_layoutable_size_request;
  iface->size_allocate = gtk_vbox_layoutable_size_allocate;
}

static void
gtk_vbox_add_layoutable_interface ()
{
  static const GInterfaceInfo layoutable_info =
  {
    (GInterfaceInitFunc) gtk_vbox_layoutable_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (gtk_vbox_get_type (),
                               GTK_TYPE_LAYOUTABLE,
                               &layoutable_info);
}

static void
gtk_vbox_layoutable_size_allocate (GtkLayoutable        *layoutable,
                                   GtkAllocation        *allocation)
{
  GtkBox *box = GTK_BOX (layoutable);
  guint pack = GTK_PACK_START;
  GList *list;
  gint i;
  gint border_width;
  gint available_width;
  GtkAllocation child_allocation;

  if (box->homogeneous)
    {
      (gtk_vbox_parent_layoutable_iface->size_allocate) (layoutable, allocation);
      return;
    }

  border_width = GTK_CONTAINER (box)->border_width;
  available_width = allocation->width - 2 * border_width;

  allocation->height = border_width;
  for (i = 0; i < 2; i++, pack = GTK_PACK_END)
    {
      list = pack == GTK_PACK_END ? g_list_last (box->children) : box->children;
      while (list)
        {
          GtkBoxChild *child_info = list->data;
          GtkLayoutable *child;
          if (child_info->pack == pack
	      && GTK_WIDGET_VISIBLE (child_info->widget))
            {
	      child = GTK_LAYOUTABLE (child_info->widget);
	      child_allocation.x = allocation->x + border_width;
	      child_allocation.y = allocation->y + allocation->height +
			           child_info->padding;
	      child_allocation.width = available_width;
	      child_allocation.height = 0;
	      gtk_layoutable_size_allocate (child, &child_allocation);

	      /* Tell the parent about our actual allocation.  */
	      allocation->width = MAX (allocation->width, child_allocation.width);
	      allocation->height += child_allocation.height + box->spacing +
				    child_info->padding * 2;
	    }

	  list = pack == GTK_PACK_END ? list->prev : list->next;
        }
    }

  if (allocation->height > border_width)
    allocation->height -= box->spacing;

  allocation->height += border_width;
}
