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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * GtkStackLayout: Widget for scrolling of arbitrary-sized areas.
 *
 * Copyright Owen Taylor, 1998
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "gtklayoutmanager.h"
#include "gtkstacklayout.h"
#include "gtkstacklayoutmarshal.h"
#include "gtklayoutadaptor.h"

#define I_(x)		(x)
#define P_(x)		(x)

enum {
   PROP_0,
   PROP_HADJUSTMENT,
   PROP_VADJUSTMENT
};

static void gtk_stack_layout_destroy (GtkObject *object);
static void gtk_stack_layout_get_property       (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void gtk_stack_layout_set_property       (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static GObject *gtk_stack_layout_constructor    (GType                  type,
					   guint                  n_properties,
					   GObjectConstructParam *properties);
static void gtk_stack_layout_child_map (GtkWidget *widget,
		                        gpointer   unused);
static void gtk_stack_layout_destroy   (GtkObject        *object);
static void gtk_stack_layout_realize            (GtkWidget      *widget);
static void gtk_stack_layout_unrealize          (GtkWidget      *widget);
static void gtk_stack_layout_map                (GtkWidget      *widget);
static void gtk_stack_layout_size_request       (GtkWidget      *widget,
                                           GtkRequisition *requisition);
static void gtk_stack_layout_size_allocate      (GtkWidget      *widget,
                                           GtkAllocation  *allocation);
static gint gtk_stack_layout_expose             (GtkWidget      *widget,
                                           GdkEventExpose *event);
static void gtk_stack_layout_add                (GtkContainer   *container,
					   GtkWidget      *widget);
static void gtk_stack_layout_remove             (GtkContainer   *container,
                                           GtkWidget      *widget);
static void gtk_stack_layout_forall             (GtkContainer   *container,
                                           gboolean        include_internals,
                                           GtkCallback     callback,
                                           gpointer        callback_data);
static void gtk_stack_layout_set_adjustments    (GtkStackLayout      *stack_layout,
                                           GtkAdjustment  *hadj,
                                           GtkAdjustment  *vadj);
static void gtk_stack_layout_adjustment_changed (GtkAdjustment  *adjustment,
                                                 GtkStackLayout      *stack_layout);
static void gtk_stack_layout_style_set          (GtkWidget      *widget,
					   GtkStyle       *old_style);

static void gtk_stack_layout_set_adjustment_upper (GtkAdjustment *adj,
					     gdouble        upper,
					     gboolean       always_emit_changed);

G_DEFINE_TYPE (GtkStackLayout, gtk_stack_layout, GTK_TYPE_CONTAINER)

/* Public interface
 */
/**
 * gtk_stack_layout_new:
 * @hadjustment: horizontal scroll adjustment, or %NULL
 * @vadjustment: vertical scroll adjustment, or %NULL
 * 
 * Creates a new #GtkStackLayout. Unless you have a specific adjustment
 * you'd like the stack_layout to use for scrolling, pass %NULL for
 * @hadjustment and @vadjustment.
 * 
 * Return value: a new #GtkStackLayout
 **/
  
GtkWidget*    
gtk_stack_layout_new (GtkAdjustment *hadjustment,
		      GtkAdjustment *vadjustment)
{
  GtkStackLayout *stack_layout;

  stack_layout = g_object_new (GTK_TYPE_STACK_LAYOUT,
			 "hadjustment", hadjustment,
			 "vadjustment", vadjustment,
			 NULL);

  return GTK_WIDGET (stack_layout);
}

/**
 * gtk_stack_layout_get_hadjustment:
 * @stack_layout: a #GtkStackLayout
 * 
 * This function should only be called after the stack_layout has been
 * placed in a #GtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #GtkAdjustment used for communication
 * between the horizontal scrollbar and @stack_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 * Return value: horizontal scroll adjustment
 **/
GtkAdjustment* 
gtk_stack_layout_get_hadjustment (GtkStackLayout     *stack_layout)
{
  g_return_val_if_fail (GTK_IS_STACK_LAYOUT (stack_layout), NULL);

  return stack_layout->hadjustment;
}
/**
 * gtk_stack_layout_get_vadjustment:
 * @stack_layout: a #GtkStackLayout
 * 
 * This function should only be called after the stack_layout has been
 * placed in a #GtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #GtkAdjustment used for communication
 * between the vertical scrollbar and @stack_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 * Return value: vertical scroll adjustment
 **/
GtkAdjustment* 
gtk_stack_layout_get_vadjustment (GtkStackLayout     *stack_layout)
{
  g_return_val_if_fail (GTK_IS_STACK_LAYOUT (stack_layout), NULL);

  return stack_layout->vadjustment;
}

static GtkAdjustment *
new_default_adjustment (void)
{
  return GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}

static void           
gtk_stack_layout_set_adjustments (GtkStackLayout     *stack_layout,
				  GtkAdjustment *hadj,
				  GtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (stack_layout));

  if (hadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else if (stack_layout->hadjustment)
    hadj = new_default_adjustment ();
  if (vadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else if (stack_layout->vadjustment)
    vadj = new_default_adjustment ();
  
  if (stack_layout->hadjustment && (stack_layout->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (stack_layout->hadjustment,
					    gtk_stack_layout_adjustment_changed,
					    stack_layout);
      g_object_unref (stack_layout->hadjustment);
    }
  
  if (stack_layout->vadjustment && (stack_layout->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (stack_layout->vadjustment,
					    gtk_stack_layout_adjustment_changed,
					    stack_layout);
      g_object_unref (stack_layout->vadjustment);
    }
  
  if (stack_layout->hadjustment != hadj)
    {
      stack_layout->hadjustment = hadj;
      g_object_ref_sink (stack_layout->hadjustment);
      gtk_stack_layout_set_adjustment_upper (stack_layout->hadjustment, stack_layout->width, FALSE);
      
      g_signal_connect (stack_layout->hadjustment, "value_changed",
			G_CALLBACK (gtk_stack_layout_adjustment_changed),
			stack_layout);
      need_adjust = TRUE;
    }
  
  if (stack_layout->vadjustment != vadj)
    {
      stack_layout->vadjustment = vadj;
      g_object_ref_sink (stack_layout->vadjustment);
      gtk_stack_layout_set_adjustment_upper (stack_layout->vadjustment, stack_layout->height, FALSE);
      
      g_signal_connect (stack_layout->vadjustment, "value_changed",
			G_CALLBACK (gtk_stack_layout_adjustment_changed),
			stack_layout);
      need_adjust = TRUE;
    }

  /* vadj or hadj can be NULL while constructing; don't emit a signal
     then */
  if (need_adjust && vadj && hadj)
    gtk_stack_layout_adjustment_changed (NULL, stack_layout);
}

static void
gtk_stack_layout_destroy (GtkObject *object)
{
  GtkStackLayout *stack_layout = GTK_STACK_LAYOUT (object);

  /* Empty the stack; the only root is retained through the root.  */
  while (stack_layout->current)
    gtk_stack_layout_pop (stack_layout);

  if (stack_layout->hadjustment)
    {
      g_object_unref (stack_layout->hadjustment);
      stack_layout->hadjustment = NULL;
    }
  if (stack_layout->vadjustment)
    {
      g_object_unref (stack_layout->vadjustment);
      stack_layout->vadjustment = NULL;
    }

  /* Keep the reference to the root, it is needed by
     gtk_stack_layout_forall.  */
  GTK_OBJECT_CLASS (gtk_stack_layout_parent_class)->destroy (object);

  /* Delete it now.  */
  if (stack_layout->root)
    {
      g_object_unref (stack_layout->root);
      stack_layout->root = NULL;
    }
}

/**
 * gtk_stack_layout_set_hadjustment:
 * @stack_layout: a #GtkStackLayout
 * @adjustment: new scroll adjustment
 *
 * Sets the horizontal scroll adjustment for the stack_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 **/
void           
gtk_stack_layout_set_hadjustment (GtkStackLayout     *stack_layout,
				  GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_STACK_LAYOUT (stack_layout));

  gtk_stack_layout_set_adjustments (stack_layout, adjustment, stack_layout->vadjustment);
  g_object_notify (G_OBJECT (stack_layout), "hadjustment");
}
 
/**
 * gtk_stack_layout_set_vadjustment:
 * @stack_layout: a #GtkStackLayout
 * @adjustment: new scroll adjustment
 *
 * Sets the vertical scroll adjustment for the stack_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 **/
void           
gtk_stack_layout_set_vadjustment (GtkStackLayout     *stack_layout,
				  GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_STACK_LAYOUT (stack_layout));
  
  gtk_stack_layout_set_adjustments (stack_layout, stack_layout->hadjustment, adjustment);
  g_object_notify (G_OBJECT (stack_layout), "vadjustment");
}

static void
gtk_stack_layout_set_adjustment_upper (GtkAdjustment *adj,
				       gdouble        upper,
				       gboolean       always_emit_changed)
{
  gboolean changed = FALSE;
  gboolean value_changed = FALSE;
  
  gdouble min = MAX (0., upper - adj->page_size);

  if (upper != adj->upper)
    {
      adj->upper = upper;
      changed = TRUE;
    }
      
  if (adj->value > min)
    {
      adj->value = min;
      value_changed = TRUE;
    }
  
  if (changed || always_emit_changed)
    gtk_adjustment_changed (adj);
  if (value_changed)
    gtk_adjustment_value_changed (adj);
}

/* Basic Object handling procedures
 */
static void
gtk_stack_layout_class_init (GtkStackLayoutClass *class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class = (GObjectClass*) class;
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;

  gobject_class->set_property = gtk_stack_layout_set_property;
  gobject_class->get_property = gtk_stack_layout_get_property;
  gobject_class->constructor = gtk_stack_layout_constructor;

  object_class->destroy = gtk_stack_layout_destroy;

  g_object_class_install_property (gobject_class,
				   PROP_HADJUSTMENT,
				   g_param_spec_object ("hadjustment",
							P_("Horizontal adjustment"),
							P_("The GtkAdjustment for the horizontal position"),
							GTK_TYPE_ADJUSTMENT,
							G_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class,
				   PROP_VADJUSTMENT,
				   g_param_spec_object ("vadjustment",
							P_("Vertical adjustment"),
							P_("The GtkAdjustment for the vertical position"),
							GTK_TYPE_ADJUSTMENT,
							G_PARAM_READWRITE));

  widget_class->realize = gtk_stack_layout_realize;
  widget_class->unrealize = gtk_stack_layout_unrealize;
  widget_class->map = gtk_stack_layout_map;
  widget_class->size_request = gtk_stack_layout_size_request;
  widget_class->size_allocate = gtk_stack_layout_size_allocate;
  widget_class->expose_event = gtk_stack_layout_expose;
  widget_class->style_set = gtk_stack_layout_style_set;

  container_class->add = gtk_stack_layout_add;
  container_class->remove = gtk_stack_layout_remove;
  container_class->forall = gtk_stack_layout_forall;

  class->set_scroll_adjustments = gtk_stack_layout_set_adjustments;

  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set_scroll_adjustments"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkStackLayoutClass, set_scroll_adjustments),
		  NULL, NULL,
		  gtk_stacklayout_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_ADJUSTMENT,
		  GTK_TYPE_ADJUSTMENT);
}

static void
gtk_stack_layout_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
  GtkStackLayout *stack_layout = GTK_STACK_LAYOUT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, stack_layout->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, stack_layout->vadjustment);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_stack_layout_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  GtkStackLayout *stack_layout = GTK_STACK_LAYOUT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      gtk_stack_layout_set_hadjustment (stack_layout, 
				  (GtkAdjustment*) g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      gtk_stack_layout_set_vadjustment (stack_layout, 
				  (GtkAdjustment*) g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_stack_layout_init (GtkStackLayout *stack_layout)
{
  stack_layout->root = NULL;

  stack_layout->width = 100;
  stack_layout->height = 100;

  stack_layout->hadjustment = NULL;
  stack_layout->vadjustment = NULL;

  stack_layout->bin_window = NULL;
}

static GObject *
gtk_stack_layout_constructor (GType                  type,
			guint                  n_properties,
			GObjectConstructParam *properties)
{
  GtkStackLayout *stack_layout;
  GObject *object;
  GtkAdjustment *hadj, *vadj;
  
  object = G_OBJECT_CLASS (gtk_stack_layout_parent_class)->constructor (type,
								  n_properties,
								  properties);

  stack_layout = GTK_STACK_LAYOUT (object);

  hadj = stack_layout->hadjustment ? stack_layout->hadjustment : new_default_adjustment ();
  vadj = stack_layout->vadjustment ? stack_layout->vadjustment : new_default_adjustment ();

  if (!stack_layout->hadjustment || !stack_layout->vadjustment)
    gtk_stack_layout_set_adjustments (stack_layout, hadj, vadj);

  return object;
}

/* Widget methods
 */

static void 
gtk_stack_layout_realize (GtkWidget *widget)
{
  GtkStackLayout *stack_layout;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (widget));

  stack_layout = GTK_STACK_LAYOUT (widget);
  GTK_WIDGET_SET_FLAGS (stack_layout, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  gdk_window_set_back_pixmap (widget->window, NULL, FALSE);
  gdk_window_set_user_data (widget->window, widget);

  attributes.x = - stack_layout->hadjustment->value,
  attributes.y = - stack_layout->vadjustment->value;
  attributes.width = MAX (stack_layout->width, widget->allocation.width);
  attributes.height = MAX (stack_layout->height, widget->allocation.height);
  attributes.event_mask = GDK_EXPOSURE_MASK | GDK_SCROLL_MASK | 
                          gtk_widget_get_events (widget);

  stack_layout->bin_window = gdk_window_new (widget->window,
					&attributes, attributes_mask);
  gdk_window_set_user_data (stack_layout->bin_window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, stack_layout->bin_window, GTK_STATE_NORMAL);

  gtk_layout_manager_foreach_widget (stack_layout->root,
				     (GtkCallback) gtk_widget_set_parent_window,
				     stack_layout->bin_window);
}

static void
gtk_stack_layout_style_set (GtkWidget *widget, GtkStyle *old_style)
{
  if (GTK_WIDGET_CLASS (gtk_stack_layout_parent_class)->style_set)
    (* GTK_WIDGET_CLASS (gtk_stack_layout_parent_class)->style_set) (widget, old_style);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gtk_style_set_background (widget->style, GTK_STACK_LAYOUT (widget)->bin_window, GTK_STATE_NORMAL);
    }
}

static void 
gtk_stack_layout_map (GtkWidget *widget)
{
  GtkStackLayout *stack_layout;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (widget));

  stack_layout = GTK_STACK_LAYOUT (widget);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

  gtk_layout_manager_foreach_widget (stack_layout->root,
				     (GtkCallback) gtk_stack_layout_child_map,
				     NULL);

  gdk_window_show (stack_layout->bin_window);
  gdk_window_show (widget->window);
}

static void
gtk_stack_layout_child_map (GtkWidget *widget,
                            gpointer   client_data)
{
  if (GTK_WIDGET_VISIBLE (widget) && !GTK_WIDGET_MAPPED (widget))
    gtk_widget_map (widget);
}

static void 
gtk_stack_layout_unrealize (GtkWidget *widget)
{
  GtkStackLayout *stack_layout;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (widget));

  stack_layout = GTK_STACK_LAYOUT (widget);

  gdk_window_set_user_data (stack_layout->bin_window, NULL);
  gdk_window_destroy (stack_layout->bin_window);
  stack_layout->bin_window = NULL;

  if (GTK_WIDGET_CLASS (gtk_stack_layout_parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (gtk_stack_layout_parent_class)->unrealize) (widget);
}

static void     
gtk_stack_layout_size_request (GtkWidget     *widget,
			       GtkRequisition *requisition)
{
  GtkStackLayout *stack_layout;
  GtkRequisition  child_requisition;
  int border_width;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (widget));

  stack_layout = GTK_STACK_LAYOUT (widget);
  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_layout_manager_size_request (stack_layout->root, &child_requisition);
  requisition->width = child_requisition.width + 2 * border_width;
  requisition->height = child_requisition.height + 2 * border_width;

  stack_layout->width = requisition->width;
  stack_layout->height = requisition->height;
}

static void     
gtk_stack_layout_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkStackLayout *stack_layout;
  GtkAllocation child_allocation;
  gint border_width;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (widget));

  stack_layout = GTK_STACK_LAYOUT (widget);
  border_width = GTK_CONTAINER (widget)->border_width;

  widget->allocation = *allocation;
  child_allocation.x = border_width;
  child_allocation.y = border_width;
  child_allocation.width = 0;
  child_allocation.height = 0;

  stack_layout->width = MAX (stack_layout->width, allocation->width);
  stack_layout->height = MAX (stack_layout->height, allocation->height);

  gtk_layout_manager_size_allocate (stack_layout->root, &child_allocation);

  stack_layout->width = MAX (child_allocation.x + child_allocation.width + border_width,
			     allocation->width);
  stack_layout->height = MAX (child_allocation.y + child_allocation.height + border_width,
			      allocation->height);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      gdk_window_resize (stack_layout->bin_window,
		         stack_layout->width, stack_layout->height);
    }

  stack_layout->hadjustment->page_size = allocation->width;
  stack_layout->hadjustment->page_increment = allocation->width * 0.9;
  stack_layout->hadjustment->lower = 0;
  /* set_adjustment_upper() emits ::changed */
  gtk_stack_layout_set_adjustment_upper (stack_layout->hadjustment,
					 stack_layout->width, TRUE);

  stack_layout->vadjustment->page_size = allocation->height;
  stack_layout->vadjustment->page_increment = allocation->height * 0.9;
  stack_layout->vadjustment->lower = 0;
  stack_layout->vadjustment->upper = stack_layout->height;
  gtk_stack_layout_set_adjustment_upper (stack_layout->vadjustment,
					 stack_layout->height, TRUE);
}

static gint 
gtk_stack_layout_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkStackLayout *stack_layout;

  g_return_val_if_fail (GTK_IS_STACK_LAYOUT (widget), FALSE);

  stack_layout = GTK_STACK_LAYOUT (widget);

  if (event->window != stack_layout->bin_window)
    return FALSE;
  
  (* GTK_WIDGET_CLASS (gtk_stack_layout_parent_class)->expose_event) (widget, event);

  return FALSE;
}

void
gtk_stack_layout_push (GtkStackLayout   *stack_layout,
		       GtkLayoutManager *manager)
{
  if (!stack_layout->current)
    {
      g_return_if_fail (stack_layout->root == NULL);
      stack_layout->root = manager;
      stack_layout->current = g_slist_prepend (stack_layout->current, manager);

      /* Sink for the root reference, add another reference for the GSList.  */
      g_object_ref_sink (manager);
      g_object_ref (manager);
    }

  else
    {
      gtk_layout_manager_add (GTK_LAYOUT_MANAGER (stack_layout->current->data),
			      manager);
      stack_layout->current = g_slist_prepend (stack_layout->current, manager);
      g_object_ref (manager);
    }

  manager->root = stack_layout;
}

void
gtk_stack_layout_pop (GtkStackLayout   *stack_layout)
{
  g_return_if_fail (stack_layout->current != NULL);

  g_object_unref (G_OBJECT (stack_layout->current->data));
  stack_layout->current = stack_layout->current->next;
}

/* Container methods
 */
static void
gtk_stack_layout_add (GtkContainer *container,
		      GtkWidget    *widget)
{
  GtkLayoutManager *adaptor = gtk_layout_adaptor_new ();
  GtkStackLayout *stack_layout = GTK_STACK_LAYOUT (container);;

  if (!stack_layout->current)
    {
      gtk_stack_layout_push (stack_layout, adaptor);
      gtk_stack_layout_pop (stack_layout);
    }
  else
    gtk_layout_manager_add (stack_layout->current->data, adaptor);

  gtk_layout_adaptor_set_child (GTK_LAYOUT_ADAPTOR (adaptor), widget);
}

static void
gtk_stack_layout_remove (GtkContainer *container, 
			 GtkWidget    *widget)
{
#if 0
  GList *tmp_list;
  GtkStackLayout *stack_layout;
  GtkStackLayoutChild *child = NULL;
  
  g_return_if_fail (GTK_IS_STACK_LAYOUT (container));
  
  stack_layout = GTK_STACK_LAYOUT (container);

  tmp_list = stack_layout->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	break;
      tmp_list = tmp_list->next;
    }

  if (tmp_list)
    {
      gtk_widget_unparent (widget);

      stack_layout->children = g_list_remove_link (stack_layout->children, tmp_list);
      g_list_free_1 (tmp_list);
      g_free (child);
    }
#endif
}

static void
gtk_stack_layout_forall (GtkContainer *container,
                         gboolean      include_internals,
			 GtkCallback   callback,
			 gpointer      callback_data)
{
  GtkStackLayout *stack_layout;

  g_return_if_fail (GTK_IS_STACK_LAYOUT (container));
  g_return_if_fail (callback != NULL);

  stack_layout = GTK_STACK_LAYOUT (container);
  if (stack_layout->root != NULL)
    gtk_layout_manager_foreach_widget (stack_layout->root,
				       callback, callback_data);
}

/* Callbacks */

static void
gtk_stack_layout_adjustment_changed (GtkAdjustment *adjustment,
				     GtkStackLayout     *stack_layout)
{
  if (GTK_WIDGET_REALIZED (stack_layout))
    {
      gdk_window_move (stack_layout->bin_window,
		       - stack_layout->hadjustment->value,
		       - stack_layout->vadjustment->value);
      
      gdk_window_process_updates (stack_layout->bin_window, TRUE);
    }
}
