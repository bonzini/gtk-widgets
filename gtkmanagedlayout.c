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
 * GtkManagedLayout: Widget for scrolling of arbitrary-sized areas.
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

#include "gtkmanagedlayout.h"
#include "gtkmanagedlayoutmarshal.h"
#include "gtklayoutable.h"

#define I_(x)		(x)
#define P_(x)		(x)

enum {
   PROP_0,
   PROP_HADJUSTMENT,
   PROP_VADJUSTMENT
};

static void gtk_managed_layout_destroy (GtkObject *object);
static void gtk_managed_layout_get_property       (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void gtk_managed_layout_set_property       (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static GObject *gtk_managed_layout_constructor    (GType                  type,
					   guint                  n_properties,
					   GObjectConstructParam *properties);
static void gtk_managed_layout_finalize   (GObject        *object);
static void gtk_managed_layout_realize            (GtkWidget      *widget);
static void gtk_managed_layout_unrealize          (GtkWidget      *widget);
static void gtk_managed_layout_map            (GtkWidget      *widget);
static void gtk_managed_layout_unmap          (GtkWidget      *widget);
static void gtk_managed_layout_size_request       (GtkWidget      *widget,
                                           GtkRequisition *requisition);
static void gtk_managed_layout_size_allocate      (GtkWidget      *widget,
                                           GtkAllocation  *allocation);
static gint gtk_managed_layout_expose             (GtkWidget      *widget,
                                           GdkEventExpose *event);
static void gtk_managed_layout_add (GtkContainer *container,
		                    GtkWidget    *child);
static void gtk_managed_layout_set_adjustments    (GtkManagedLayout      *managed_layout,
                                           GtkAdjustment  *hadj,
                                           GtkAdjustment  *vadj);
static void gtk_managed_layout_adjustment_changed (GtkAdjustment  *adjustment,
                                                 GtkManagedLayout      *managed_layout);
static void gtk_managed_layout_style_set          (GtkWidget      *widget,
					   GtkStyle       *old_style);

static void gtk_managed_layout_set_adjustment_upper (GtkAdjustment *adj,
						     gdouble        upper,
						     gboolean       always_emit_changed);

G_DEFINE_TYPE (GtkManagedLayout, gtk_managed_layout, GTK_TYPE_BIN)

/* Public interface
 */
/**
 * gtk_managed_layout_new:
 * @hadjustment: horizontal scroll adjustment, or %NULL
 * @vadjustment: vertical scroll adjustment, or %NULL
 * 
 * Creates a new #GtkManagedLayout. Unless you have a specific adjustment
 * you'd like the managed_layout to use for scrolling, pass %NULL for
 * @hadjustment and @vadjustment.
 * 
 * Return value: a new #GtkManagedLayout
 **/
  
GtkWidget*    
gtk_managed_layout_new (GtkAdjustment *hadjustment,
		      GtkAdjustment *vadjustment)
{
  GtkManagedLayout *managed_layout;

  managed_layout = g_object_new (GTK_TYPE_MANAGED_LAYOUT,
			 "hadjustment", hadjustment,
			 "vadjustment", vadjustment,
			 NULL);

  return GTK_WIDGET (managed_layout);
}

/**
 * gtk_managed_layout_get_hadjustment:
 * @managed_layout: a #GtkManagedLayout
 * 
 * This function should only be called after the managed_layout has been
 * placed in a #GtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #GtkAdjustment used for communication
 * between the horizontal scrollbar and @managed_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 * Return value: horizontal scroll adjustment
 **/
GtkAdjustment* 
gtk_managed_layout_get_hadjustment (GtkManagedLayout     *managed_layout)
{
  g_return_val_if_fail (GTK_IS_MANAGED_LAYOUT (managed_layout), NULL);

  return managed_layout->hadjustment;
}
/**
 * gtk_managed_layout_get_vadjustment:
 * @managed_layout: a #GtkManagedLayout
 * 
 * This function should only be called after the managed_layout has been
 * placed in a #GtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #GtkAdjustment used for communication
 * between the vertical scrollbar and @managed_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 * Return value: vertical scroll adjustment
 **/
GtkAdjustment* 
gtk_managed_layout_get_vadjustment (GtkManagedLayout     *managed_layout)
{
  g_return_val_if_fail (GTK_IS_MANAGED_LAYOUT (managed_layout), NULL);

  return managed_layout->vadjustment;
}

static GtkAdjustment *
new_default_adjustment (void)
{
  return GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}

static void           
gtk_managed_layout_set_adjustments (GtkManagedLayout     *managed_layout,
				  GtkAdjustment *hadj,
				  GtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (managed_layout));

  if (hadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else if (managed_layout->hadjustment)
    hadj = new_default_adjustment ();
  if (vadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else if (managed_layout->vadjustment)
    vadj = new_default_adjustment ();
  
  if (managed_layout->hadjustment && (managed_layout->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (managed_layout->hadjustment,
					    gtk_managed_layout_adjustment_changed,
					    managed_layout);
      g_object_unref (managed_layout->hadjustment);
    }
  
  if (managed_layout->vadjustment && (managed_layout->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (managed_layout->vadjustment,
					    gtk_managed_layout_adjustment_changed,
					    managed_layout);
      g_object_unref (managed_layout->vadjustment);
    }
  
  if (managed_layout->hadjustment != hadj)
    {
      managed_layout->hadjustment = hadj;
      g_object_ref_sink (managed_layout->hadjustment);
      gtk_managed_layout_set_adjustment_upper (managed_layout->hadjustment, managed_layout->width, FALSE);
      
      g_signal_connect (managed_layout->hadjustment, "value_changed",
			G_CALLBACK (gtk_managed_layout_adjustment_changed),
			managed_layout);
      need_adjust = TRUE;
    }
  
  if (managed_layout->vadjustment != vadj)
    {
      managed_layout->vadjustment = vadj;
      g_object_ref_sink (managed_layout->vadjustment);
      gtk_managed_layout_set_adjustment_upper (managed_layout->vadjustment, managed_layout->height, FALSE);
      
      g_signal_connect (managed_layout->vadjustment, "value_changed",
			G_CALLBACK (gtk_managed_layout_adjustment_changed),
			managed_layout);
      need_adjust = TRUE;
    }

  /* vadj or hadj can be NULL while constructing; don't emit a signal
     then */
  if (need_adjust && vadj && hadj)
    gtk_managed_layout_adjustment_changed (NULL, managed_layout);
}

static void
gtk_managed_layout_finalize (GObject *object)
{
  GtkManagedLayout *managed_layout = GTK_MANAGED_LAYOUT (object);

  if (managed_layout->hadjustment)
    {
      g_object_unref (managed_layout->hadjustment);
      managed_layout->hadjustment = NULL;
    }
  if (managed_layout->vadjustment)
    {
      g_object_unref (managed_layout->vadjustment);
      managed_layout->vadjustment = NULL;
    }

  G_OBJECT_CLASS (gtk_managed_layout_parent_class)->finalize (object);
}

static void
gtk_managed_layout_destroy (GtkObject *object)
{
  GtkManagedLayout *managed_layout = GTK_MANAGED_LAYOUT (object);

  if (managed_layout->hadjustment)
    {
      g_object_unref (managed_layout->hadjustment);
      managed_layout->hadjustment = NULL;
    }
  if (managed_layout->vadjustment)
    {
      g_object_unref (managed_layout->vadjustment);
      managed_layout->vadjustment = NULL;
    }

  GTK_OBJECT_CLASS (gtk_managed_layout_parent_class)->destroy (object);
}

/**
 * gtk_managed_layout_set_hadjustment:
 * @managed_layout: a #GtkManagedLayout
 * @adjustment: new scroll adjustment
 *
 * Sets the horizontal scroll adjustment for the managed_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 **/
void           
gtk_managed_layout_set_hadjustment (GtkManagedLayout     *managed_layout,
				  GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (managed_layout));

  gtk_managed_layout_set_adjustments (managed_layout, adjustment, managed_layout->vadjustment);
  g_object_notify (G_OBJECT (managed_layout), "hadjustment");
}
 
/**
 * gtk_managed_layout_set_vadjustment:
 * @managed_layout: a #GtkManagedLayout
 * @adjustment: new scroll adjustment
 *
 * Sets the vertical scroll adjustment for the managed_layout.
 *
 * See #GtkScrolledWindow, #GtkScrollbar, #GtkAdjustment for details.
 * 
 **/
void           
gtk_managed_layout_set_vadjustment (GtkManagedLayout     *managed_layout,
				  GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (managed_layout));
  
  gtk_managed_layout_set_adjustments (managed_layout, managed_layout->hadjustment, adjustment);
  g_object_notify (G_OBJECT (managed_layout), "vadjustment");
}

static void
gtk_managed_layout_set_adjustment_upper (GtkAdjustment *adj,
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
gtk_managed_layout_class_init (GtkManagedLayoutClass *class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class = (GObjectClass*) class;
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;

  gobject_class->set_property = gtk_managed_layout_set_property;
  gobject_class->get_property = gtk_managed_layout_get_property;
  gobject_class->constructor = gtk_managed_layout_constructor;
  gobject_class->finalize = gtk_managed_layout_finalize;

  object_class->destroy = gtk_managed_layout_destroy;

  container_class->add = gtk_managed_layout_add;

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

  widget_class->realize = gtk_managed_layout_realize;
  widget_class->unrealize = gtk_managed_layout_unrealize;
  widget_class->map = gtk_managed_layout_map;
  widget_class->unmap = gtk_managed_layout_unmap;
  widget_class->size_request = gtk_managed_layout_size_request;
  widget_class->size_allocate = gtk_managed_layout_size_allocate;
  widget_class->expose_event = gtk_managed_layout_expose;
  widget_class->style_set = gtk_managed_layout_style_set;

  class->set_scroll_adjustments = gtk_managed_layout_set_adjustments;

  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set_scroll_adjustments"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkManagedLayoutClass, set_scroll_adjustments),
		  NULL, NULL,
		  gtk_managedlayout_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  GTK_TYPE_ADJUSTMENT,
		  GTK_TYPE_ADJUSTMENT);
}

static void
gtk_managed_layout_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
  GtkManagedLayout *managed_layout = GTK_MANAGED_LAYOUT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, managed_layout->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, managed_layout->vadjustment);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_managed_layout_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  GtkManagedLayout *managed_layout = GTK_MANAGED_LAYOUT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      gtk_managed_layout_set_hadjustment (managed_layout, 
				  (GtkAdjustment*) g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      gtk_managed_layout_set_vadjustment (managed_layout, 
				  (GtkAdjustment*) g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_managed_layout_init (GtkManagedLayout *managed_layout)
{
  GTK_WIDGET_UNSET_FLAGS (managed_layout, GTK_NO_WINDOW);

  managed_layout->width = 100;
  managed_layout->height = 100;
  managed_layout->requested_width = -1;
  managed_layout->requested_height = -1;

  managed_layout->hadjustment = NULL;
  managed_layout->vadjustment = NULL;

  managed_layout->bin_window = NULL;
}

static GObject *
gtk_managed_layout_constructor (GType                  type,
			guint                  n_properties,
			GObjectConstructParam *properties)
{
  GtkManagedLayout *managed_layout;
  GObject *object;
  GtkAdjustment *hadj, *vadj;
  
  object = G_OBJECT_CLASS (gtk_managed_layout_parent_class)->constructor (type,
								  n_properties,
								  properties);

  managed_layout = GTK_MANAGED_LAYOUT (object);

  hadj = managed_layout->hadjustment ? managed_layout->hadjustment : new_default_adjustment ();
  vadj = managed_layout->vadjustment ? managed_layout->vadjustment : new_default_adjustment ();

  if (!managed_layout->hadjustment || !managed_layout->vadjustment)
    gtk_managed_layout_set_adjustments (managed_layout, hadj, vadj);

  return object;
}

/* Widget methods
 */

static void 
gtk_managed_layout_realize (GtkWidget *widget)
{
  GtkManagedLayout *managed_layout;
  GtkBin *bin;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (widget));

  bin = GTK_BIN (widget);
  managed_layout = GTK_MANAGED_LAYOUT (widget);
  GTK_WIDGET_SET_FLAGS (managed_layout, GTK_REALIZED);

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

  attributes.x = - managed_layout->hadjustment->value,
  attributes.y = - managed_layout->vadjustment->value;
  attributes.width = MAX (managed_layout->width, widget->allocation.width);
  attributes.height = MAX (managed_layout->height, widget->allocation.height);
  attributes.event_mask = GDK_EXPOSURE_MASK | GDK_SCROLL_MASK | 
                          gtk_widget_get_events (widget);

  managed_layout->bin_window = gdk_window_new (widget->window,
					&attributes, attributes_mask);
  gdk_window_set_user_data (managed_layout->bin_window, widget);

  if (bin->child)
    gtk_widget_set_parent_window (bin->child, managed_layout->bin_window);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, managed_layout->bin_window, GTK_STATE_NORMAL);
}

 
static void 
gtk_managed_layout_map (GtkWidget *widget)
{
  GtkManagedLayout *managed_layout;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (widget));

  managed_layout = GTK_MANAGED_LAYOUT (widget);
  gdk_window_show (managed_layout->bin_window);
  gdk_window_show (widget->window);

  GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->map (widget);
}

static void 
gtk_managed_layout_unmap (GtkWidget *widget)
{
  GtkManagedLayout *managed_layout;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (widget));

  managed_layout = GTK_MANAGED_LAYOUT (widget);
  gdk_window_hide (managed_layout->bin_window);
  gdk_window_hide (widget->window);

  GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->map (widget);
}

static void
gtk_managed_layout_style_set (GtkWidget *widget, GtkStyle *old_style)
{
  if (GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->style_set)
    (* GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->style_set) (widget, old_style);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gtk_style_set_background (widget->style, GTK_MANAGED_LAYOUT (widget)->bin_window, GTK_STATE_NORMAL);
    }
}

static void 
gtk_managed_layout_unrealize (GtkWidget *widget)
{
  GtkManagedLayout *managed_layout;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (widget));

  managed_layout = GTK_MANAGED_LAYOUT (widget);

  gdk_window_set_user_data (managed_layout->bin_window, NULL);
  gdk_window_destroy (managed_layout->bin_window);
  managed_layout->bin_window = NULL;

  if (GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->unrealize) (widget);
}

static void     
gtk_managed_layout_size_request (GtkWidget     *widget,
			       GtkRequisition *requisition)
{
  GtkBin *bin;
  GtkLayoutable *child;
  GtkManagedLayout *managed_layout;
  GtkRequisition child_requisition;
  int border_width;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (widget));

  bin = GTK_BIN (widget);
  managed_layout = GTK_MANAGED_LAYOUT (widget);

  child = GTK_LAYOUTABLE (bin->child);
  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_layoutable_size_request (child, &child_requisition);
  managed_layout->width = child_requisition.width + 2 * border_width;
  managed_layout->height = child_requisition.height + 2 * border_width;
  managed_layout->requested_width = widget->allocation.width;
  managed_layout->requested_height = widget->allocation.height;

  requisition->width = 0;
  requisition->height = 0;
}

static void     
gtk_managed_layout_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkBin *bin;
  GtkLayoutable *child;
  GtkManagedLayout *managed_layout;
  GtkAllocation child_allocation;
  gint border_width;
  gboolean size_changed;

  g_return_if_fail (GTK_IS_MANAGED_LAYOUT (widget));

  bin = GTK_BIN (widget);
  managed_layout = GTK_MANAGED_LAYOUT (widget);

  child = GTK_LAYOUTABLE (bin->child);
  border_width = GTK_CONTAINER (widget)->border_width;

  size_changed =
    (managed_layout->requested_width != allocation->width ||
     managed_layout->requested_height != allocation->height);
  
  widget->allocation = *allocation;
  if (size_changed)
    gtk_widget_queue_resize (widget);

  managed_layout->width = MAX (managed_layout->width, allocation->width);
  managed_layout->height = MAX (managed_layout->height, allocation->height);

  child_allocation.x = border_width;
  child_allocation.y = border_width;
  child_allocation.width = managed_layout->width - 2 * border_width;
  child_allocation.height = 0;

  gtk_layoutable_size_allocate (child, &child_allocation);

  managed_layout->width = MAX (child_allocation.x + child_allocation.width + border_width,
			     allocation->width);
  managed_layout->height = MAX (child_allocation.y + child_allocation.height + border_width,
			      allocation->height);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      gdk_window_resize (managed_layout->bin_window,
		         managed_layout->width, managed_layout->height);
    }

  managed_layout->hadjustment->page_size = allocation->width;
  managed_layout->hadjustment->page_increment = allocation->width * 0.9;
  managed_layout->hadjustment->lower = 0;
  /* set_adjustment_upper() emits ::changed */
  gtk_managed_layout_set_adjustment_upper (managed_layout->hadjustment,
					 managed_layout->width, TRUE);

  managed_layout->vadjustment->page_size = allocation->height;
  managed_layout->vadjustment->page_increment = allocation->height * 0.9;
  managed_layout->vadjustment->lower = 0;
  managed_layout->vadjustment->upper = managed_layout->height;
  gtk_managed_layout_set_adjustment_upper (managed_layout->vadjustment,
					 managed_layout->height, TRUE);
}

static gint 
gtk_managed_layout_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkManagedLayout *managed_layout;

  g_return_val_if_fail (GTK_IS_MANAGED_LAYOUT (widget), FALSE);

  managed_layout = GTK_MANAGED_LAYOUT (widget);

  if (event->window != managed_layout->bin_window)
    return FALSE;
  
  (* GTK_WIDGET_CLASS (gtk_managed_layout_parent_class)->expose_event) (widget, event);

  return FALSE;
}

static void
gtk_managed_layout_add (GtkContainer *container,
	                GtkWidget    *child)
{
  GtkBin *bin;

  g_return_if_fail (GTK_IS_WIDGET (child));

  bin = GTK_BIN (container);

  if (bin->child == NULL && GTK_WIDGET_REALIZED (container))
    gtk_widget_set_parent_window (child, GTK_MANAGED_LAYOUT (bin)->bin_window);

  GTK_CONTAINER_CLASS (gtk_managed_layout_parent_class)->add (container, child);
}


/* Callbacks */

static void
gtk_managed_layout_adjustment_changed (GtkAdjustment *adjustment,
				     GtkManagedLayout     *managed_layout)
{
  if (GTK_WIDGET_REALIZED (managed_layout))
    {
      gdk_window_move (managed_layout->bin_window,
		       - managed_layout->hadjustment->value,
		       - managed_layout->vadjustment->value);
      
      gdk_window_process_updates (managed_layout->bin_window, TRUE);
    }
}
