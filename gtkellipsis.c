/* GtkEllipsis widget.
 *
 * Copyright (C) 2008 Free Software Foundation, Inc.
 * Written by Paolo Bonzini.
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
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <assert.h>
#include "gtkellipsis.h"

#define GTK_ELLIPSIS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTK_TYPE_ELLIPSIS, GtkEllipsisPrivate))

#define GTK_ELLIPSIS_CURSOR	    GDK_HAND2
#define GTK_ELLIPSIS_IN_SPACING	    2
#define GTK_ELLIPSIS_OUT_SPACING    2
#define GTK_ELLIPSIS_SPACING	    (GTK_ELLIPSIS_IN_SPACING + GTK_ELLIPSIS_OUT_SPACING)
#define GTK_ELLIPSIS_DEFAULT_WRAP   PANGO_WRAP_CHAR

#define I_(x)		(x)
#define P_(x)		(x)

enum
{
  PROP_0,
  PROP_EXPANDED,
  PROP_LABEL,
  PROP_WRAP_MODE,
  PROP_USE_UNDERLINE,
  PROP_USE_MARKUP,
  PROP_LABEL_WIDGET
};

struct _GtkEllipsisPrivate
{
  GtkWidget        *label;
  GtkWidget        *ellipsis_label;
  GdkWindow        *event_window;

  guint             expand_timer;

  PangoWrapMode     wrap_mode;

  guint             expanded : 1;
  guint             use_underline : 1;
  guint             use_markup : 1; 
  guint             button_down : 1;
  guint             prelight : 1;
};

static void gtk_ellipsis_set_property (GObject          *object,
				       guint             prop_id,
				       const GValue     *value,
				       GParamSpec       *pspec);
static void gtk_ellipsis_get_property (GObject          *object,
				       guint             prop_id,
				       GValue           *value,
				       GParamSpec       *pspec);

static void     gtk_ellipsis_realize        (GtkWidget        *widget);
static void     gtk_ellipsis_unrealize      (GtkWidget        *widget);
static void     gtk_ellipsis_size_request   (GtkWidget        *widget,
					     GtkRequisition   *requisition);
static void     gtk_ellipsis_size_allocate  (GtkWidget        *widget,
					     GtkAllocation    *allocation);
static void     gtk_ellipsis_map            (GtkWidget        *widget);
static void     gtk_ellipsis_unmap          (GtkWidget        *widget);
static gboolean gtk_ellipsis_expose         (GtkWidget        *widget,
					     GdkEventExpose   *event);
static gboolean gtk_ellipsis_button_press   (GtkWidget        *widget,
					     GdkEventButton   *event);
static gboolean gtk_ellipsis_button_release (GtkWidget        *widget,
					     GdkEventButton   *event);
static gboolean gtk_ellipsis_enter_notify   (GtkWidget        *widget,
					     GdkEventCrossing *event);
static gboolean gtk_ellipsis_leave_notify   (GtkWidget        *widget,
					     GdkEventCrossing *event);
static gboolean gtk_ellipsis_focus          (GtkWidget        *widget,
					     GtkDirectionType  direction);
static void     gtk_ellipsis_grab_notify    (GtkWidget        *widget,
					     gboolean          was_grabbed);
static void     gtk_ellipsis_state_changed  (GtkWidget        *widget,
					     GtkStateType      previous_state);
static gboolean gtk_ellipsis_drag_motion    (GtkWidget        *widget,
					     GdkDragContext   *context,
					     gint              x,
					     gint              y,
					     guint             time);
static void     gtk_ellipsis_drag_leave     (GtkWidget        *widget,
					     GdkDragContext   *context,
					     guint             time);

static void gtk_ellipsis_add    (GtkContainer *container,
				 GtkWidget    *widget);
static void gtk_ellipsis_remove (GtkContainer *container,
				 GtkWidget    *widget);
static void gtk_ellipsis_forall (GtkContainer *container,
				 gboolean        include_internals,
				 GtkCallback     callback,
				 gpointer        callback_data);

static void gtk_ellipsis_activate (GtkEllipsis *ellipsis);

/* GtkBuildable */
static void gtk_ellipsis_buildable_init           (GtkBuildableIface *iface);
static void gtk_ellipsis_buildable_add_child      (GtkBuildable *buildable,
						   GtkBuilder   *builder,
						   GObject      *child,
						   const gchar  *type);

G_DEFINE_TYPE_WITH_CODE (GtkEllipsis, gtk_ellipsis, GTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
						gtk_ellipsis_buildable_init))

static void
gtk_ellipsis_class_init (GtkEllipsisClass *klass)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  gobject_class   = (GObjectClass *) klass;
  object_class    = (GtkObjectClass *) klass;
  widget_class    = (GtkWidgetClass *) klass;
  container_class = (GtkContainerClass *) klass;

  gobject_class->set_property = gtk_ellipsis_set_property;
  gobject_class->get_property = gtk_ellipsis_get_property;

  widget_class->realize              = gtk_ellipsis_realize;
  widget_class->unrealize            = gtk_ellipsis_unrealize;
  widget_class->size_request         = gtk_ellipsis_size_request;
  widget_class->size_allocate        = gtk_ellipsis_size_allocate;
  widget_class->map                  = gtk_ellipsis_map;
  widget_class->unmap                = gtk_ellipsis_unmap;
  widget_class->expose_event         = gtk_ellipsis_expose;
  widget_class->button_press_event   = gtk_ellipsis_button_press;
  widget_class->button_release_event = gtk_ellipsis_button_release;
  widget_class->enter_notify_event   = gtk_ellipsis_enter_notify;
  widget_class->leave_notify_event   = gtk_ellipsis_leave_notify;
  widget_class->focus                = gtk_ellipsis_focus;
  widget_class->grab_notify          = gtk_ellipsis_grab_notify;
  widget_class->state_changed        = gtk_ellipsis_state_changed;
  widget_class->drag_motion          = gtk_ellipsis_drag_motion;
  widget_class->drag_leave           = gtk_ellipsis_drag_leave;

  container_class->add    = gtk_ellipsis_add;
  container_class->remove = gtk_ellipsis_remove;
  container_class->forall = gtk_ellipsis_forall;

  klass->activate = gtk_ellipsis_activate;

  g_type_class_add_private (klass, sizeof (GtkEllipsisPrivate));

  g_object_class_install_property (gobject_class,
				   PROP_EXPANDED,
				   g_param_spec_boolean ("expanded",
							 P_("Expanded"),
							 P_("Whether the ellipsis has been opened to reveal the child widget"),
							 FALSE,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
				   PROP_LABEL,
				   g_param_spec_string ("label",
							P_("Label"),
							P_("Text of the ellipsis's label"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_WRAP_MODE,
                                   g_param_spec_enum ("wrap-mode",
                                                      P_("Line wrap mode"),
                                                      P_("If wrap is set, controls how linewrapping is done"),
                                                      PANGO_TYPE_WRAP_MODE,
                                                      GTK_ELLIPSIS_DEFAULT_WRAP,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_USE_UNDERLINE,
				   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
							 FALSE,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
				   PROP_USE_MARKUP,
				   g_param_spec_boolean ("use-markup",
							 P_("Use markup"),
							 P_("The text of the label includes XML markup. See pango_parse_markup()"),
							 FALSE,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
				   PROP_LABEL_WIDGET,
				   g_param_spec_object ("label-widget",
							P_("Label widget"),
							P_("A widget to display in place of the usual label"),
							GTK_TYPE_WIDGET,
							G_PARAM_READWRITE));

  widget_class->activate_signal =
    g_signal_new (I_("activate"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GtkEllipsisClass, activate),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
gtk_ellipsis_init (GtkEllipsis *ellipsis)
{
  GtkEllipsisPrivate *priv;

  ellipsis->priv = priv = GTK_ELLIPSIS_GET_PRIVATE (ellipsis);

  GTK_WIDGET_SET_FLAGS (ellipsis, GTK_CAN_FOCUS);
  GTK_WIDGET_SET_FLAGS (ellipsis, GTK_NO_WINDOW);

  priv->label = NULL;
  priv->ellipsis_label = NULL;
  priv->event_window = NULL;

  priv->wrap_mode = GTK_ELLIPSIS_DEFAULT_WRAP;
  priv->expanded = FALSE;
  priv->use_underline = FALSE;
  priv->use_markup = FALSE;
  priv->button_down = FALSE;
  priv->prelight = FALSE;
  priv->expand_timer = 0;

  gtk_drag_dest_set (GTK_WIDGET (ellipsis), 0, NULL, 0, 0);
  gtk_drag_dest_set_track_motion (GTK_WIDGET (ellipsis), TRUE);
}

static void
gtk_ellipsis_buildable_add_child (GtkBuildable  *buildable,
				  GtkBuilder    *builder,
				  GObject       *child,
				  const gchar   *type)
{
  if (!type)
    gtk_container_add (GTK_CONTAINER (buildable), GTK_WIDGET (child));
  else if (strcmp (type, "label") == 0)
    gtk_ellipsis_set_label_widget (GTK_ELLIPSIS (buildable), GTK_WIDGET (child));
  else
    GTK_BUILDER_WARN_INVALID_CHILD_TYPE (GTK_ELLIPSIS (buildable), type);
}

static void
gtk_ellipsis_buildable_init (GtkBuildableIface *iface)
{
  iface->add_child = gtk_ellipsis_buildable_add_child;
}

static void
gtk_ellipsis_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (object);
                                                                                                             
  switch (prop_id)
    {
    case PROP_EXPANDED:
      gtk_ellipsis_set_expanded (ellipsis, g_value_get_boolean (value));
      break;
    case PROP_LABEL:
      gtk_ellipsis_set_label (ellipsis, g_value_get_string (value));
      break;
    case PROP_WRAP_MODE:
      gtk_ellipsis_set_line_wrap_mode (ellipsis, g_value_get_enum (value));
      break;
    case PROP_USE_UNDERLINE:
      gtk_ellipsis_set_use_underline (ellipsis, g_value_get_boolean (value));
      break;
    case PROP_USE_MARKUP:
      gtk_ellipsis_set_use_markup (ellipsis, g_value_get_boolean (value));
      break;
    case PROP_LABEL_WIDGET:
      gtk_ellipsis_set_label_widget (ellipsis, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_ellipsis_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (object);
  GtkEllipsisPrivate *priv = ellipsis->priv;

  switch (prop_id)
    {
    case PROP_EXPANDED:
      g_value_set_boolean (value, priv->expanded);
      break;
    case PROP_LABEL:
      g_value_set_string (value, gtk_ellipsis_get_label (ellipsis));
      break;
    case PROP_WRAP_MODE:
      g_value_set_enum (value, priv->wrap_mode);
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, priv->use_markup);
      break;
    case PROP_LABEL_WIDGET:
      g_value_set_object (value,
			  priv->label ?
			  G_OBJECT (priv->label) : NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gint
get_label_line_height (GtkWidget *label)
{
  PangoLayout *layout;
  PangoLayoutIter *iter;
  PangoRectangle rect;

  layout = gtk_label_get_layout (GTK_LABEL (label));
  iter = pango_layout_get_iter (layout);
  pango_layout_iter_get_line_extents (iter, NULL, &rect);
  pango_layout_iter_free (iter);

  return (rect.y + rect.height) / PANGO_SCALE;
}

static void
gtk_ellipsis_realize (GtkWidget *widget)
{
  GtkEllipsis *ellipsis;
  GtkEllipsisPrivate *priv;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  gint label_height;

  ellipsis = GTK_ELLIPSIS (widget);
  priv = ellipsis->priv;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  border_width = GTK_CONTAINER (widget)->border_width;

  if (priv->label && GTK_WIDGET_VISIBLE (priv->label))
    {
      gint focus_width, focus_pad;

      gtk_widget_style_get (widget,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL);

      label_height = get_label_line_height (priv->label);
      label_height = MIN (label_height,
                          widget->allocation.height - 2 * border_width -
                          2 * focus_width - 2 * focus_pad);
    }
  else
    label_height = 0;

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = MAX (widget->allocation.width - 2 * border_width, 1);
  attributes.height = MAX (label_height, 1);
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget)     |
				GDK_BUTTON_PRESS_MASK        |
				GDK_BUTTON_RELEASE_MASK      |
				GDK_ENTER_NOTIFY_MASK        |
				GDK_LEAVE_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;
  if (GTK_WIDGET_IS_SENSITIVE (widget))
    {
      attributes.cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                                      GTK_ELLIPSIS_CURSOR);
      attributes_mask |= GDK_WA_CURSOR;
    }

  widget->window = gtk_widget_get_parent_window (widget);
  g_object_ref (widget->window);

  priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
				       &attributes, attributes_mask);
  gdk_window_set_user_data (priv->event_window, widget);
  if (attributes_mask & GDK_WA_CURSOR)
    gdk_cursor_unref (attributes.cursor);

  widget->style = gtk_style_attach (widget->style, widget->window);

  if (priv->label && GTK_WIDGET_VISIBLE (priv->label) && !priv->expanded)
    gdk_window_show (priv->event_window);
}

static void
gtk_ellipsis_unrealize (GtkWidget *widget)
{
  GtkEllipsisPrivate *priv = GTK_ELLIPSIS (widget)->priv;

  if (priv->event_window)
    {
      gdk_window_set_user_data (priv->event_window, NULL);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (gtk_ellipsis_parent_class)->unrealize (widget);
}

static void
gtk_ellipsis_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  GtkEllipsis *ellipsis;
  GtkBin *bin;
  GtkEllipsisPrivate *priv;
  gint border_width;
  gint focus_width;
  gint focus_pad;

  bin = GTK_BIN (widget);
  ellipsis = GTK_ELLIPSIS (widget);
  priv = ellipsis->priv;

  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  if (priv->label && GTK_WIDGET_VISIBLE (priv->label) && !priv->expanded)
    {
      GtkRequisition label_requisition;
      GtkRequisition ellipsis_requisition;

      requisition->width = 2 * focus_width + 2 * focus_pad;
      requisition->height = 2 * focus_width + 2 * focus_pad;

      gtk_widget_size_request (priv->label, &label_requisition);
      requisition->width  += label_requisition.width;
      requisition->height += get_label_line_height (priv->label);

      if (priv->ellipsis_label && GTK_WIDGET_VISIBLE (priv->ellipsis_label))
	{
          gtk_widget_size_request (priv->ellipsis_label, &ellipsis_requisition);
          requisition->width += ellipsis_requisition.width + GTK_ELLIPSIS_SPACING;
	}
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }

  if (bin->child && priv->expanded)
    {
      GtkRequisition child_requisition;

      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width = MAX (requisition->width, child_requisition.width);
      requisition->height += child_requisition.height;
    }

  requisition->width  += 2 * border_width;
  requisition->height += 2 * border_width;
}

static void
gtk_ellipsis_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GtkEllipsis *ellipsis;
  GtkBin *bin;
  GtkEllipsisPrivate *priv;
  GtkRequisition child_requisition;
  gint border_width;
  gint focus_width;
  gint focus_pad;

  ellipsis = GTK_ELLIPSIS (widget);
  bin = GTK_BIN (widget);
  priv = ellipsis->priv;

  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  child_requisition.width = 0;
  child_requisition.height = 0;
  if (bin->child && priv->expanded)
    gtk_widget_get_child_requisition (bin->child, &child_requisition);

  widget->allocation = *allocation;

  if (priv->label && GTK_WIDGET_VISIBLE (priv->label) && !priv->expanded)
    {
      GtkAllocation label_allocation;
      GtkAllocation ellipsis_allocation;
      GtkRequisition ellipsis_requisition;
      PangoLayout *layout;
      gboolean ltr;

      label_allocation.y = (widget->allocation.y + border_width
			    + focus_width + focus_pad);
      label_allocation.width = MAX (1,
				    allocation->width - 2 * border_width -
				    GTK_ELLIPSIS_SPACING -
				    2 * focus_width - 2 * focus_pad);

      label_allocation.height = get_label_line_height (priv->label);
      label_allocation.height = MIN (label_allocation.height,
				     allocation->height - 2 * border_width -
				     2 * focus_width - 2 * focus_pad);
      label_allocation.height = MAX (label_allocation.height, 1);

      ltr = gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL;

      if (priv->ellipsis_label && GTK_WIDGET_VISIBLE (priv->label))
        {
          gtk_widget_get_child_requisition (priv->ellipsis_label,
					    &ellipsis_requisition);

          if (ltr)
            ellipsis_allocation.x = widget->allocation.x
				    + widget->allocation.width - border_width
			            - GTK_ELLIPSIS_OUT_SPACING
			            - ellipsis_requisition.width;
          else
            ellipsis_allocation.x = widget->allocation.x
				    + border_width
            			    + GTK_ELLIPSIS_OUT_SPACING;

          ellipsis_allocation.y = label_allocation.y;
          ellipsis_allocation.width = ellipsis_requisition.width;
          ellipsis_allocation.height = ellipsis_requisition.height;

	  gtk_widget_size_allocate (priv->ellipsis_label,
				    &ellipsis_allocation);

          label_allocation.width = MAX (1, label_allocation.width -
				        ellipsis_allocation.width);
        }

      if (ltr)
	label_allocation.x = (widget->allocation.x +
                              border_width + focus_width + focus_pad);
      else
        label_allocation.x = (widget->allocation.x + widget->allocation.width -
                              (label_allocation.width +
                               border_width + focus_width + focus_pad));

      /* HACK!  We know that GtkLabel only sets the PangoLayout's width
	 in its size_request method, so we are free to do that here.
	 Together with setting the X alignment to 0.0, this truncates
	 the label to whatever fits in the first line.  */
      layout = gtk_label_get_layout (GTK_LABEL (priv->label));
      pango_layout_set_width (layout, label_allocation.width * PANGO_SCALE);
      gtk_widget_size_allocate (priv->label, &label_allocation);

      if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize (priv->event_window,
			        allocation->x + border_width, 
			        allocation->y + border_width, 
			        MAX (allocation->width - 2 * border_width, 1), 
			        MAX (label_allocation.height, 1));
    }

  if (priv->expanded)
    {
      GtkAllocation child_allocation;

      child_allocation.x = widget->allocation.x + border_width;
      child_allocation.y = widget->allocation.y + border_width;
      child_allocation.width = MAX (allocation->width - 2 * border_width, 1);
      child_allocation.height = MAX (allocation->height - 2 * border_width, 1);
      gtk_widget_size_allocate (bin->child, &child_allocation);
    }

}

static void
gtk_ellipsis_map (GtkWidget *widget)
{
  GtkEllipsisPrivate *priv = GTK_ELLIPSIS (widget)->priv;

  if (priv->label)
    {
      gtk_widget_map (priv->label);
      gtk_widget_map (priv->ellipsis_label);
    }

  GTK_WIDGET_CLASS (gtk_ellipsis_parent_class)->map (widget);

  if (priv->event_window)
    gdk_window_show (priv->event_window);
}

static void
gtk_ellipsis_unmap (GtkWidget *widget)
{
  GtkEllipsisPrivate *priv = GTK_ELLIPSIS (widget)->priv;

  if (priv->event_window)
    gdk_window_hide (priv->event_window);

  GTK_WIDGET_CLASS (gtk_ellipsis_parent_class)->unmap (widget);

  if (priv->label)
    {
      gtk_widget_unmap (priv->ellipsis_label);
      gtk_widget_unmap (priv->label);
    }
}

static void
gtk_ellipsis_paint (GtkEllipsis *ellipsis)
{
  GtkWidget *widget;
  GtkContainer *container;
  GtkEllipsisPrivate *priv;
  GtkStateType state;
  GdkRectangle area;
  int focus_width;
  int focus_pad;

  widget = GTK_WIDGET (ellipsis);
  container = GTK_CONTAINER (ellipsis);
  priv = ellipsis->priv;

  if (priv->expanded)
    return;

  state = widget->state;
  if (priv->prelight && state != GTK_STATE_SELECTED)
    state = GTK_STATE_PRELIGHT;

  if (state == GTK_STATE_NORMAL)
    return;

  gtk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  area.x = widget->allocation.x + container->border_width;
  area.y = widget->allocation.y + container->border_width;
  area.width = widget->allocation.width - (2 * container->border_width);
  area.height = (focus_width + focus_pad) * 2;

  if (priv->label && GTK_WIDGET_VISIBLE (priv->label))
    area.height += priv->label->allocation.height;

  gtk_paint_flat_box (widget->style, widget->window,
		      state,
		      GTK_SHADOW_ETCHED_OUT,
		      &area, widget, "ellipsis",
		      area.x, area.y,
		      area.width, area.height);
}

static void
gtk_ellipsis_paint_focus (GtkEllipsis  *ellipsis,
			  GdkRectangle *area)
{
  GtkWidget *widget;
  GtkEllipsisPrivate *priv;
  gint x, y, width, height;
  gint border_width;
  gint focus_width;
  gint focus_pad;
  gboolean ltr;

  widget = GTK_WIDGET (ellipsis);
  priv = ellipsis->priv;
  if (priv->expanded)
    return;

  border_width = GTK_CONTAINER (widget)->border_width;

  gtk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  ltr = gtk_widget_get_direction (widget) != GTK_TEXT_DIR_RTL;
  
  width = height = 0;

  if (priv->label)
    {
      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;
      width  = widget->allocation.width - 2 * border_width;

      if (GTK_WIDGET_VISIBLE (priv->label))
	{
	  GtkAllocation label_allocation = priv->label->allocation;
	  height = label_allocation.height;
	}

      height += 2 * focus_pad + 2 * focus_width;
    }
      
  gtk_paint_focus (widget->style, widget->window, GTK_WIDGET_STATE (widget),
		   area, widget, "expander",
		   x, y, width, height);
}

static gboolean
gtk_ellipsis_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);

      gtk_ellipsis_paint (ellipsis);

      if (GTK_WIDGET_HAS_FOCUS (ellipsis))
	gtk_ellipsis_paint_focus (ellipsis, &event->area);

      GTK_WIDGET_CLASS (gtk_ellipsis_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static gboolean
gtk_ellipsis_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);

  if (event->button == 1 && event->window == ellipsis->priv->event_window)
    {
      ellipsis->priv->button_down = TRUE;
      return TRUE;
    }

  return FALSE;
}

static gboolean
gtk_ellipsis_button_release (GtkWidget      *widget,
			     GdkEventButton *event)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);

  if (event->button == 1 && ellipsis->priv->button_down)
    {
      gtk_widget_activate (widget);
      ellipsis->priv->button_down = FALSE;
      return TRUE;
    }

  return FALSE;
}

static void
gtk_ellipsis_grab_notify (GtkWidget *widget,
			  gboolean   was_grabbed)
{
  if (!was_grabbed)
    GTK_ELLIPSIS (widget)->priv->button_down = FALSE;
}

static void
gtk_ellipsis_state_changed (GtkWidget    *widget,
			    GtkStateType  previous_state)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);
  GtkEllipsisPrivate *priv = ellipsis->priv;

  if (!GTK_WIDGET_IS_SENSITIVE (widget))
    priv->button_down = FALSE;

  if (GTK_WIDGET_REALIZED (widget))
    {
      GdkCursor *cursor;

      if (GTK_WIDGET_IS_SENSITIVE (widget))
        cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                             GTK_ELLIPSIS_CURSOR);
      else
        cursor = NULL;

      gdk_window_set_cursor (priv->event_window, cursor);
      if (cursor)
        gdk_cursor_unref (cursor);
    }
}

static void
gtk_ellipsis_redraw_ellipsis (GtkEllipsis *ellipsis)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (ellipsis);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
}

static gboolean
gtk_ellipsis_enter_notify (GtkWidget        *widget,
			   GdkEventCrossing *event)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);
  GtkWidget *event_widget;

  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if (event_widget == widget &&
      event->detail != GDK_NOTIFY_INFERIOR)
    {
      ellipsis->priv->prelight = TRUE;

      if (ellipsis->priv->label
	  && widget->state != GTK_STATE_SELECTED)
        {
	  gtk_widget_set_state (ellipsis->priv->label, GTK_STATE_PRELIGHT);
	  gtk_widget_set_state (ellipsis->priv->ellipsis_label, GTK_STATE_PRELIGHT);
        }

      gtk_ellipsis_redraw_ellipsis (ellipsis);
    }

  return FALSE;
}

static gboolean
gtk_ellipsis_leave_notify (GtkWidget        *widget,
			   GdkEventCrossing *event)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);
  GtkWidget *event_widget;

  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if (event_widget == widget &&
      event->detail != GDK_NOTIFY_INFERIOR)
    {
      ellipsis->priv->prelight = FALSE;

      if (ellipsis->priv->label
	  && widget->state != GTK_STATE_SELECTED)
	{
	  gtk_widget_set_state (ellipsis->priv->label, GTK_STATE_NORMAL);
	  gtk_widget_set_state (ellipsis->priv->ellipsis_label, GTK_STATE_NORMAL);
	}

      gtk_ellipsis_redraw_ellipsis (ellipsis);
    }

  return FALSE;
}

static gboolean
expand_timeout (gpointer data)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (data);
  GtkEllipsisPrivate *priv = ellipsis->priv;

  priv->expand_timer = 0;
  gtk_ellipsis_set_expanded (ellipsis, TRUE);

  return FALSE;
}

static gboolean
gtk_ellipsis_drag_motion (GtkWidget        *widget,
			  GdkDragContext   *context,
			  gint              x,
			  gint              y,
			  guint             time)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);
  GtkEllipsisPrivate *priv = ellipsis->priv;

  if (!priv->expanded && !priv->expand_timer)
    {
      GtkSettings *settings;
      guint timeout;

      settings = gtk_widget_get_settings (widget);
      g_object_get (settings, "gtk-timeout-expand", &timeout, NULL);

      priv->expand_timer = gdk_threads_add_timeout (timeout, (GSourceFunc) expand_timeout, ellipsis);
    }

  return TRUE;
}

static void
gtk_ellipsis_drag_leave (GtkWidget      *widget,
			 GdkDragContext *context,
			 guint           time)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);
  GtkEllipsisPrivate *priv = ellipsis->priv;

  if (priv->expand_timer)
    {
      g_source_remove (priv->expand_timer);
      priv->expand_timer = 0;
    }
}

typedef enum
{
  FOCUS_NONE,
  FOCUS_WIDGET,
  FOCUS_LABEL,
  FOCUS_CHILD
} FocusSite;

static gboolean
focus_current_site (GtkEllipsis      *ellipsis,
		    GtkDirectionType  direction)
{
  GtkWidget *current_focus;

  current_focus = GTK_CONTAINER (ellipsis)->focus_child;

  if (!current_focus)
    return FALSE;

  return gtk_widget_child_focus (current_focus, direction);
}

static gboolean
focus_in_site (GtkEllipsis      *ellipsis,
	       FocusSite         site,
	       GtkDirectionType  direction)
{
  GtkEllipsisPrivate *priv = ellipsis->priv;
  switch (site)
    {
    case FOCUS_WIDGET:
      gtk_widget_grab_focus (GTK_WIDGET (ellipsis));
      return TRUE;
    case FOCUS_LABEL:
      if (priv->label && GTK_WIDGET_VISIBLE (priv->label))
	return gtk_widget_child_focus (priv->label, direction);
      else
	return FALSE;
    case FOCUS_CHILD:
      {
	GtkWidget *child = gtk_bin_get_child (GTK_BIN (ellipsis));

	if (child && priv->expanded)
	  return gtk_widget_child_focus (child, direction);
	else
	  return FALSE;
      }
    case FOCUS_NONE:
      break;
    }

  g_assert_not_reached ();
  return FALSE;
}

static FocusSite
get_next_site (GtkEllipsis      *ellipsis,
	       FocusSite         site,
	       GtkDirectionType  direction)
{
  gboolean ltr;

  ltr = gtk_widget_get_direction (GTK_WIDGET (ellipsis)) != GTK_TEXT_DIR_RTL;

  switch (site)
    {
    case FOCUS_NONE:
      switch (direction)
	{
	case GTK_DIR_TAB_BACKWARD:
	case GTK_DIR_LEFT:
	case GTK_DIR_UP:
	  return FOCUS_CHILD;
	case GTK_DIR_TAB_FORWARD:
	case GTK_DIR_DOWN:
	case GTK_DIR_RIGHT:
	  return FOCUS_WIDGET;
	}
    case FOCUS_WIDGET:
      switch (direction)
	{
	case GTK_DIR_TAB_BACKWARD:
	case GTK_DIR_UP:
	  return FOCUS_NONE;
	case GTK_DIR_LEFT:
	  return ltr ? FOCUS_NONE : FOCUS_LABEL;
	case GTK_DIR_TAB_FORWARD:
	case GTK_DIR_DOWN:
	  return FOCUS_LABEL;
	case GTK_DIR_RIGHT:
	  return ltr ? FOCUS_LABEL : FOCUS_NONE;
	  break;
	}
    case FOCUS_LABEL:
      switch (direction)
	{
	case GTK_DIR_TAB_BACKWARD:
	case GTK_DIR_UP:
	  return FOCUS_WIDGET;
	case GTK_DIR_LEFT:
	  return ltr ? FOCUS_WIDGET : FOCUS_CHILD;
	case GTK_DIR_TAB_FORWARD:
	case GTK_DIR_DOWN:
	  return FOCUS_CHILD;
	case GTK_DIR_RIGHT:
	  return ltr ? FOCUS_CHILD : FOCUS_WIDGET;
	  break;
	}
    case FOCUS_CHILD:
      switch (direction)
	{
	case GTK_DIR_TAB_BACKWARD:
	case GTK_DIR_LEFT:
	case GTK_DIR_UP:
	  return FOCUS_LABEL;
	case GTK_DIR_TAB_FORWARD:
	case GTK_DIR_DOWN:
	case GTK_DIR_RIGHT:
	  return FOCUS_NONE;
	}
    }

  g_assert_not_reached ();
  return FOCUS_NONE;
}

static gboolean
gtk_ellipsis_focus (GtkWidget        *widget,
		    GtkDirectionType  direction)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (widget);
  
  if (!focus_current_site (ellipsis, direction))
    {
      GtkWidget *old_focus_child;
      gboolean widget_is_focus;
      FocusSite site = FOCUS_NONE;
      
      widget_is_focus = gtk_widget_is_focus (widget);
      old_focus_child = GTK_CONTAINER (widget)->focus_child;
      
      if (old_focus_child && old_focus_child == ellipsis->priv->label)
	site = FOCUS_LABEL;
      else if (old_focus_child)
	site = FOCUS_CHILD;
      else if (widget_is_focus)
	site = FOCUS_WIDGET;

      while ((site = get_next_site (ellipsis, site, direction)) != FOCUS_NONE)
	{
	  if (focus_in_site (ellipsis, site, direction))
	    return TRUE;
	}

      return FALSE;
    }

  return TRUE;
}

static void
gtk_ellipsis_add (GtkContainer *container,
		  GtkWidget    *widget)
{
  GTK_CONTAINER_CLASS (gtk_ellipsis_parent_class)->add (container, widget);

  gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
gtk_ellipsis_remove (GtkContainer *container,
		     GtkWidget    *widget)
{
  GtkEllipsis *ellipsis = GTK_ELLIPSIS (container);

  if (GTK_ELLIPSIS (ellipsis)->priv->label == widget)
    gtk_ellipsis_set_label_widget (ellipsis, NULL);
  else if (GTK_ELLIPSIS (ellipsis)->priv->ellipsis_label == widget)
    ;
  else
    GTK_CONTAINER_CLASS (gtk_ellipsis_parent_class)->remove (container, widget);
}

static void
gtk_ellipsis_forall (GtkContainer *container,
		     gboolean      include_internals,
		     GtkCallback   callback,
		     gpointer      callback_data)
{
  GtkBin *bin = GTK_BIN (container);
  GtkEllipsisPrivate *priv = GTK_ELLIPSIS (container)->priv;

  if (bin->child)
    (* callback) (bin->child, callback_data);

  if (priv->label)
    (* callback) (priv->label, callback_data);

  if (priv->ellipsis_label)
    (* callback) (priv->ellipsis_label, callback_data);

}

static void
gtk_ellipsis_activate (GtkEllipsis *ellipsis)
{
  gtk_ellipsis_set_expanded (ellipsis, !ellipsis->priv->expanded);
}

/**
 * gtk_ellipsis_new:
 * @label: the text of the label
 * 
 * Creates a new ellipsis using @label as the text of the label.
 * 
 * Return value: a new #GtkEllipsis widget.
 *
 * Since: 2.4
 **/
GtkWidget *
gtk_ellipsis_new (const gchar *label)
{
  return g_object_new (GTK_TYPE_ELLIPSIS, "label", label, NULL);
}

/**
 * gtk_ellipsis_new_with_mnemonic:
 * @label: the text of the label with an underscore in front of the
 *         mnemonic character
 * 
 * Creates a new ellipsis using @label as the text of the label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use '__' (two 
 * underscores). The first underlined character represents a keyboard 
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 * 
 * Return value: a new #GtkEllipsis widget.
 *
 * Since: 2.4
 **/
GtkWidget *
gtk_ellipsis_new_with_mnemonic (const gchar *label)
{
  return g_object_new (GTK_TYPE_ELLIPSIS,
		       "label", label,
		       "use-underline", TRUE,
		       NULL);
}

/**
 * gtk_ellipsis_set_expanded:
 * @ellipsis: a #GtkEllipsis
 * @expanded: whether the child widget is revealed
 *
 * Sets the state of the ellipsis. Set to %TRUE, if you want
 * the child widget to be revealed, and %FALSE if you want the
 * child widget to be hidden.
 *
 * Since: 2.4
 **/
void
gtk_ellipsis_set_expanded (GtkEllipsis *ellipsis,
			   gboolean     expanded)
{
  GtkEllipsisPrivate *priv;

  g_return_if_fail (GTK_IS_ELLIPSIS (ellipsis));

  priv = ellipsis->priv;

  expanded = expanded != FALSE;

  if (priv->expanded != expanded)
    {
      GtkWidget *child = GTK_BIN (ellipsis)->child;
      priv->expanded = expanded;

      if (child)
	{
          if (!expanded && GTK_WIDGET_MAPPED (child))
	    gtk_widget_unmap (child);
          if (expanded && GTK_WIDGET_MAPPED (priv->label))
	    {
              if (GTK_WIDGET_REALIZED (ellipsis))
	        gdk_window_hide (priv->event_window);
	      gtk_widget_unmap (priv->ellipsis_label);
	      gtk_widget_unmap (priv->label);
	    }

          if (GTK_WIDGET_MAPPED (ellipsis))
	    {
	      if (expanded && GTK_WIDGET_VISIBLE (child))
		gtk_widget_map (child);
	      if (!expanded && GTK_WIDGET_VISIBLE (priv->label))
		{
		  gtk_widget_map (priv->label);
		  gtk_widget_map (priv->ellipsis_label);
                  if (GTK_WIDGET_REALIZED (ellipsis))
		    gdk_window_show (priv->event_window);
		}
	    }

	  gtk_widget_queue_resize (GTK_WIDGET (ellipsis));
	}

      g_object_notify (G_OBJECT (ellipsis), "expanded");
    }
}

/**
 * gtk_ellipsis_get_expanded:
 * @ellipsis:a #GtkEllipsis
 *
 * Queries a #GtkEllipsis and returns its current state. Returns %TRUE
 * if the child widget is revealed.
 *
 * See gtk_ellipsis_set_expanded().
 *
 * Return value: the current state of the ellipsis.
 *
 * Since: 2.4
 **/
gboolean
gtk_ellipsis_get_expanded (GtkEllipsis *ellipsis)
{
  g_return_val_if_fail (GTK_IS_ELLIPSIS (ellipsis), FALSE);

  return ellipsis->priv->expanded;
}

/**
 * gtk_ellipsis_set_label:
 * @ellipsis: a #GtkEllipsis
 * @label: a string
 *
 * Sets the text of the label of the ellipsis to @label.
 *
 * This will also clear any previously set labels.
 *
 * Since: 2.4
 **/
void
gtk_ellipsis_set_label (GtkEllipsis *ellipsis,
			const gchar *label)
{
  g_return_if_fail (GTK_IS_ELLIPSIS (ellipsis));

  if (!label)
    {
      gtk_ellipsis_set_label_widget (ellipsis, NULL);
    }
  else
    {
      GtkWidget *child;

      child = gtk_label_new (label);
      gtk_label_set_line_wrap (GTK_LABEL (child), TRUE);
      gtk_label_set_line_wrap_mode (GTK_LABEL (child), ellipsis->priv->wrap_mode);
      gtk_label_set_use_underline (GTK_LABEL (child), ellipsis->priv->use_underline);
      gtk_label_set_use_markup (GTK_LABEL (child), ellipsis->priv->use_markup);
      gtk_widget_show (child);

      gtk_ellipsis_set_label_widget (ellipsis, child);
    }

  g_object_notify (G_OBJECT (ellipsis), "label");
}

/**
 * gtk_ellipsis_get_label:
 * @ellipsis: a #GtkEllipsis
 *
 * Fetches the text from the label of the ellipsis, as set by
 * gtk_ellipsis_set_label(). If the label text has not
 * been set the return value will be %NULL. This will be the
 * case if you create an empty button with gtk_button_new() to
 * use as a container.
 *
 * Return value: The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 *
 * Since: 2.4
 **/
G_CONST_RETURN char *
gtk_ellipsis_get_label (GtkEllipsis *ellipsis)
{
  GtkEllipsisPrivate *priv;

  g_return_val_if_fail (GTK_IS_ELLIPSIS (ellipsis), NULL);

  priv = ellipsis->priv;

  if (priv->label && GTK_IS_LABEL (priv->label))
    return gtk_label_get_text (GTK_LABEL (priv->label));
  else
    return NULL;
}

/**
 * gtk_ellipsis_set_line_wrap_mode:
 * @ellipsis: a #GtkEllipsis
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * Controls how the line wrapping is done.
 *
 * Since: 2.10
 **/
void
gtk_ellipsis_set_line_wrap_mode (GtkEllipsis *ellipsis,
				 PangoWrapMode wrap_mode)
{
  GtkEllipsisPrivate *priv;

  g_return_if_fail (GTK_IS_ELLIPSIS (ellipsis));

  priv = ellipsis->priv;

  if (priv->wrap_mode != wrap_mode)
    {
      priv->wrap_mode = wrap_mode;

      if (priv->label && GTK_IS_LABEL (priv->label))
	gtk_label_set_line_wrap_mode (GTK_LABEL (priv->label), wrap_mode);

      g_object_notify (G_OBJECT (ellipsis), "wrap-mode");
    }
}

/**
 * gtk_ellipsis_get_line_wrap_mode:
 * @ellipsis: a #GtkEllipsis
 *
 * Returns how the line wrapping is done.
 *
 * Return value: A #PangoWrapMode indicating where lines can be broken.
 *
 * Since: 2.4
 **/
PangoWrapMode
gtk_ellipsis_get_line_wrap_mode (GtkEllipsis *ellipsis)
{
  g_return_val_if_fail (GTK_IS_ELLIPSIS (ellipsis), FALSE);

  return ellipsis->priv->wrap_mode;
}

/**
 * gtk_ellipsis_set_use_underline:
 * @ellipsis: a #GtkEllipsis
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the ellipsis label indicates
 * the next character should be used for the mnemonic accelerator key.
 *
 * Since: 2.4
 **/
void
gtk_ellipsis_set_use_underline (GtkEllipsis *ellipsis,
				gboolean     use_underline)
{
  GtkEllipsisPrivate *priv;

  g_return_if_fail (GTK_IS_ELLIPSIS (ellipsis));

  priv = ellipsis->priv;

  use_underline = use_underline != FALSE;

  if (priv->use_underline != use_underline)
    {
      priv->use_underline = use_underline;

      if (priv->label && GTK_IS_LABEL (priv->label))
	gtk_label_set_use_underline (GTK_LABEL (priv->label), use_underline);

      g_object_notify (G_OBJECT (ellipsis), "use-underline");
    }
}

/**
 * gtk_ellipsis_get_use_underline:
 * @ellipsis: a #GtkEllipsis
 *
 * Returns whether an embedded underline in the ellipsis label indicates a
 * mnemonic. See gtk_ellipsis_set_use_underline().
 *
 * Return value: %TRUE if an embedded underline in the ellipsis label
 *               indicates the mnemonic accelerator keys.
 *
 * Since: 2.4
 **/
gboolean
gtk_ellipsis_get_use_underline (GtkEllipsis *ellipsis)
{
  g_return_val_if_fail (GTK_IS_ELLIPSIS (ellipsis), FALSE);

  return ellipsis->priv->use_underline;
}

/**
 * gtk_ellipsis_set_use_markup:
 * @ellipsis: a #GtkEllipsis
 * @use_markup: %TRUE if the label's text should be parsed for markup
 *
 * Sets whether the text of the label contains markup in <link
 * linkend="PangoMarkupFormat">Pango's text markup
 * language</link>. See gtk_label_set_markup().
 *
 * Since: 2.4
 **/
void
gtk_ellipsis_set_use_markup (GtkEllipsis *ellipsis,
			     gboolean     use_markup)
{
  GtkEllipsisPrivate *priv;

  g_return_if_fail (GTK_IS_ELLIPSIS (ellipsis));

  priv = ellipsis->priv;

  use_markup = use_markup != FALSE;

  if (priv->use_markup != use_markup)
    {
      priv->use_markup = use_markup;

      if (priv->label && GTK_IS_LABEL (priv->label))
	gtk_label_set_use_markup (GTK_LABEL (priv->label), use_markup);

      g_object_notify (G_OBJECT (ellipsis), "use-markup");
    }
}

/**
 * gtk_ellipsis_get_use_markup:
 * @ellipsis: a #GtkEllipsis
 *
 * Returns whether the label's text is interpreted as marked up with
 * the <link linkend="PangoMarkupFormat">Pango text markup
 * language</link>. See gtk_ellipsis_set_use_markup ().
 *
 * Return value: %TRUE if the label's text will be parsed for markup
 *
 * Since: 2.4
 **/
gboolean
gtk_ellipsis_get_use_markup (GtkEllipsis *ellipsis)
{
  g_return_val_if_fail (GTK_IS_ELLIPSIS (ellipsis), FALSE);

  return ellipsis->priv->use_markup;
}

/**
 * gtk_ellipsis_set_label_widget:
 * @ellipsis: a #GtkEllipsis
 * @label: the new label widget
 *
 * Set the label widget for the ellipsis. This is the widget
 * that will appear embedded alongside the ellipsis arrow.
 *
 * Since: 2.4
 **/
void
gtk_ellipsis_set_label_widget (GtkEllipsis *ellipsis,
			       GtkWidget   *label)
{
  GtkEllipsisPrivate *priv;

  g_return_if_fail (GTK_IS_ELLIPSIS (ellipsis));
  g_return_if_fail (label == NULL || GTK_IS_WIDGET (label));
  g_return_if_fail (label == NULL || label->parent == NULL);

  priv = ellipsis->priv;

  if (priv->label == label)
    return;

  if (priv->label)
    {
      gtk_widget_set_state (priv->label, GTK_STATE_NORMAL);
      gtk_widget_unparent (priv->label);
    }

  priv->label = label;

  if (label)
    {
      gfloat xalign, yalign;
      gtk_misc_get_alignment (&GTK_LABEL (label)->misc, &xalign, &yalign);
      gtk_misc_set_alignment (&GTK_LABEL (label)->misc, 0.0, yalign);
      gtk_widget_set_parent (label, GTK_WIDGET (ellipsis));

      if (!priv->ellipsis_label)
	{
	  GdkColor *link_color, active_bg_color;
	  GtkWidget *child;
          PangoAttrList *attrs;
          PangoAttribute *uline;
          child = gtk_label_new ("...");

	  /* Change the foreground color for all states but selected.  */
          gtk_widget_style_get (GTK_WIDGET (child),
                                "link-color", &link_color, NULL);
	  if (!link_color)
	    {
              GtkStyle *style = gtk_widget_get_style (child);
	      active_bg_color = style->bg[GTK_STATE_SELECTED];
	      link_color = &active_bg_color;
	    }
	  gtk_widget_modify_fg (child, GTK_STATE_NORMAL, link_color);
	  gtk_widget_modify_fg (child, GTK_STATE_ACTIVE, link_color);
	  gtk_widget_modify_fg (child, GTK_STATE_PRELIGHT, link_color);
	  if (link_color != &active_bg_color)
	    gdk_color_free (link_color);

	  /* Add an underline.  */
          attrs = gtk_label_get_attributes (GTK_LABEL (label));
	  attrs = attrs ? pango_attr_list_copy (attrs) : pango_attr_list_new ();

          uline = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          uline->start_index = 0;
          uline->end_index = G_MAXUINT;
	  pango_attr_list_insert (attrs, uline);
	  gtk_label_set_attributes (GTK_LABEL (child), attrs);
	  pango_attr_list_unref (attrs);

          gtk_widget_show (child);
          gtk_widget_set_parent (child, GTK_WIDGET (ellipsis));
	  priv->ellipsis_label = child;
	}

      if (priv->prelight)
	{
	  gtk_widget_set_state (label, GTK_STATE_PRELIGHT);
	  gtk_widget_set_state (priv->ellipsis_label, GTK_STATE_PRELIGHT);
	}
    }
  else
    {
      if (priv->ellipsis_label)
        {
          gtk_widget_destroy (priv->ellipsis_label);
          priv->ellipsis_label = NULL;
        }
    }

  if (GTK_WIDGET_VISIBLE (ellipsis))
    gtk_widget_queue_resize (GTK_WIDGET (ellipsis));

  g_object_freeze_notify (G_OBJECT (ellipsis));
  g_object_notify (G_OBJECT (ellipsis), "label-widget");
  g_object_notify (G_OBJECT (ellipsis), "label");
  g_object_thaw_notify (G_OBJECT (ellipsis));
}

/**
 * gtk_ellipsis_get_label_widget:
 * @ellipsis: a #GtkEllipsis
 *
 * Retrieves the label widget for the frame. See
 * gtk_ellipsis_set_label_widget().
 *
 * Return value: the label widget, or %NULL if there is none.
 * 
 * Since: 2.4
 **/
GtkWidget *
gtk_ellipsis_get_label_widget (GtkEllipsis *ellipsis)
{
  g_return_val_if_fail (GTK_IS_ELLIPSIS (ellipsis), NULL);

  return ellipsis->priv->label;
}
