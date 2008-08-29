/* GtkResizer widget.
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gtkresizermarshal.h"
#include "gtkresizer.h"

#define GTK_RESIZER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GTK_TYPE_RESIZER, GtkResizerPrivate))

#define GTK_RESIZER_EDGE		GDK_WINDOW_EDGE_SOUTH
#define GTK_RESIZER_CURSOR		GDK_SB_V_DOUBLE_ARROW

#define I_(x)		(x)
#define P_(x)		(x)

enum {
  TOGGLE_HANDLE_FOCUS,
  MOVE_HANDLE,
  ACCEPT_POSITION,
  CANCEL_POSITION,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SIZE,
  PROP_SIZE_SET,
  PROP_MIN_SIZE,
  PROP_MAX_SIZE
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_SHRINK
};


struct _GtkResizerPrivate
{
  GdkWindow *handle;
  GdkRectangle handle_pos;

  gint size;
  gint last_allocation;
  gint min_size;
  gint max_size;

  guint size_set : 1;
  guint in_drag : 1;
  guint shrink : 1;
  guint handle_prelit : 1;

  GtkWidget *last_child_focus;

  gint drag_pos;
  gint original_size;
  guint32 grab_time;
};

static void gtk_resizer_set_property (GObject          *object,
				       guint             prop_id,
				       const GValue     *value,
				       GParamSpec       *pspec);
static void gtk_resizer_get_property (GObject          *object,
				       guint             prop_id,
				       GValue           *value,
				       GParamSpec       *pspec);
static void gtk_resizer_set_child_property (GtkContainer    *container,
		                            GtkWidget       *child,
		                            guint            property_id,
		                            const GValue    *value,
		                            GParamSpec      *pspec);
static void gtk_resizer_get_child_property (GtkContainer    *object,
		                            GtkWidget       *child,
					    guint            prop_id,
					    GValue          *value,
					    GParamSpec      *pspec);

static void     gtk_resizer_realize        (GtkWidget        *widget);
static void     gtk_resizer_unrealize      (GtkWidget        *widget);
static void     gtk_resizer_size_request   (GtkWidget        *widget,
					    GtkRequisition   *requisition);
static void     gtk_resizer_size_allocate  (GtkWidget        *widget,
					    GtkAllocation    *allocation);
static void     gtk_resizer_map            (GtkWidget        *widget);
static void     gtk_resizer_unmap          (GtkWidget        *widget);
static gboolean gtk_resizer_expose         (GtkWidget        *widget,
					    GdkEventExpose   *event);
static gboolean gtk_resizer_grab_broken    (GtkWidget          *widget,
			                    GdkEventGrabBroken *event);
static gboolean gtk_resizer_button_press   (GtkWidget        *widget,
					    GdkEventButton   *event);
static gboolean gtk_resizer_motion	   (GtkWidget        *widget,
					    GdkEventMotion   *event);
static gboolean gtk_resizer_button_release (GtkWidget        *widget,
					    GdkEventButton   *event);
static gboolean gtk_resizer_enter_notify   (GtkWidget        *widget,
					    GdkEventCrossing *event);
static gboolean gtk_resizer_leave_notify   (GtkWidget        *widget,
					    GdkEventCrossing *event);
static gboolean gtk_resizer_focus          (GtkWidget        *widget,
					    GtkDirectionType  direction);
static void     gtk_resizer_grab_notify    (GtkWidget        *widget,
					    gboolean          was_grabbed);
static void     gtk_resizer_state_changed  (GtkWidget        *widget,
					    GtkStateType      previous_state);
static gboolean	gtk_resizer_toggle_handle_focus  (GtkResizer        *widget);
static gboolean	gtk_resizer_accept_size          (GtkResizer        *widget);
static gboolean	gtk_resizer_cancel_size          (GtkResizer        *widget);
static gboolean gtk_resizer_move_handle		 (GtkResizer        *resizer,
				                  GtkScrollType      scroll);

G_DEFINE_TYPE (GtkResizer, gtk_resizer, GTK_TYPE_BIN)

static void
add_tab_bindings (GtkBindingSet    *binding_set,
                  GdkModifierType   modifiers)
{
  gtk_binding_entry_add_signal (binding_set, GDK_Tab, modifiers,
                                "toggle_handle_focus", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Tab, modifiers,
                                "toggle_handle_focus", 0);
}

static void
add_move_binding (GtkBindingSet   *binding_set,
                  guint            keyval,
                  GdkModifierType  mask,
                  GtkScrollType    scroll)
{
  gtk_binding_entry_add_signal (binding_set, keyval, mask,
                                "move_handle", 1,
                                GTK_TYPE_SCROLL_TYPE, scroll);
}

static guint signals[LAST_SIGNAL] = { 0 };

static void
gtk_resizer_class_init (GtkResizerClass *klass)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GtkBindingSet *binding_set;

  gobject_class   = (GObjectClass *) klass;
  object_class    = (GtkObjectClass *) klass;
  widget_class    = (GtkWidgetClass *) klass;
  container_class = (GtkContainerClass *) klass;

  gobject_class->set_property = gtk_resizer_set_property;
  gobject_class->get_property = gtk_resizer_get_property;

  widget_class->realize              = gtk_resizer_realize;
  widget_class->unrealize            = gtk_resizer_unrealize;
  widget_class->size_request         = gtk_resizer_size_request;
  widget_class->size_allocate        = gtk_resizer_size_allocate;
  widget_class->map                  = gtk_resizer_map;
  widget_class->unmap                = gtk_resizer_unmap;
  widget_class->expose_event         = gtk_resizer_expose;
  widget_class->button_press_event   = gtk_resizer_button_press;
  widget_class->button_release_event = gtk_resizer_button_release;
  widget_class->motion_notify_event  = gtk_resizer_motion;
  widget_class->enter_notify_event   = gtk_resizer_enter_notify;
  widget_class->leave_notify_event   = gtk_resizer_leave_notify;
  widget_class->grab_broken_event    = gtk_resizer_grab_broken;
  widget_class->focus                = gtk_resizer_focus;
  widget_class->grab_notify          = gtk_resizer_grab_notify;
  widget_class->state_changed        = gtk_resizer_state_changed;

  container_class->set_child_property = gtk_resizer_set_child_property;
  container_class->get_child_property = gtk_resizer_get_child_property;

  klass->toggle_handle_focus = gtk_resizer_toggle_handle_focus;
  klass->move_handle = gtk_resizer_move_handle;
  klass->accept_size = gtk_resizer_accept_size;
  klass->cancel_size = gtk_resizer_cancel_size;

  g_type_class_add_private (klass, sizeof (GtkResizerPrivate));

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_int ("size",
                                                     P_("Size"),
                                                     P_("Size of resizer child in pixels"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_SIZE_SET,
                                   g_param_spec_boolean ("size-set",
                                                         P_("Size Set"),
                                                         P_("TRUE if the Size property should be used"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
                                   
  g_object_class_install_property (gobject_class,
                                   PROP_MIN_SIZE,
                                   g_param_spec_int ("min-size",
                                                     P_("Minimal Size"),
                                                     P_("Smallest possible value for the \"size\" property"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
                                   PROP_MAX_SIZE,
                                   g_param_spec_int ("max-size",
                                                     P_("Maximal Size"),
                                                     P_("Largest possible value for the \"size\" property"),
                                                     0,
                                                     G_MAXINT,
                                                     G_MAXINT,
                                                     G_PARAM_READABLE));

  gtk_container_class_install_child_property (container_class,
                                              CHILD_PROP_SHRINK,
                                              g_param_spec_boolean ("shrink", 
                                                                    P_("Shrink"),
                                                                    P_("If TRUE, the child can be made smaller than its requisition"),
                                                                    TRUE,
                                                                    G_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("handle-size",
                                                             P_("Handle Size"),
                                                             P_("Width of handle"),
                                                             0,
                                                             G_MAXINT,
                                                             5,
                                                             G_PARAM_READABLE));

  signals [TOGGLE_HANDLE_FOCUS] =
    g_signal_new (I_("toggle_handle_focus"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GtkResizerClass, toggle_handle_focus),
                  NULL, NULL,
                  gtk_resizer_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  signals[MOVE_HANDLE] =
    g_signal_new (I_("move_handle"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GtkResizerClass, move_handle),
                  NULL, NULL,
                  gtk_resizer_marshal_BOOLEAN__ENUM,
                  G_TYPE_BOOLEAN, 1,
                  GTK_TYPE_SCROLL_TYPE);

  signals [ACCEPT_POSITION] =
    g_signal_new (I_("accept_size"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GtkResizerClass, accept_size),
                  NULL, NULL,
                  gtk_resizer_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  signals [CANCEL_POSITION] =
    g_signal_new (I_("cancel_size"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GtkResizerClass, cancel_size),
                  NULL, NULL,
                  gtk_resizer_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  binding_set = gtk_binding_set_by_class (klass);
  add_tab_bindings (binding_set, 0);
  add_tab_bindings (binding_set, GDK_CONTROL_MASK);
  add_tab_bindings (binding_set, GDK_SHIFT_MASK);
  add_tab_bindings (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  /* accept and cancel sizes */
  gtk_binding_entry_add_signal (binding_set,
                                GDK_Escape, 0,
                                "cancel_size", 0);

  gtk_binding_entry_add_signal (binding_set,
                                GDK_Return, 0,
                                "accept_size", 0);
  gtk_binding_entry_add_signal (binding_set,
                                GDK_ISO_Enter, 0,
                                "accept_size", 0);
  gtk_binding_entry_add_signal (binding_set,
                                GDK_KP_Enter, 0,
                                "accept_size", 0);
  gtk_binding_entry_add_signal (binding_set,
                                GDK_space, 0,
                                "accept_size", 0);
  gtk_binding_entry_add_signal (binding_set,
                                GDK_KP_Space, 0,
                                "accept_size", 0);

  /* move handle */
  add_move_binding (binding_set, GDK_Left, 0, GTK_SCROLL_STEP_LEFT);
  add_move_binding (binding_set, GDK_KP_Left, 0, GTK_SCROLL_STEP_LEFT);

  add_move_binding (binding_set, GDK_Right, 0, GTK_SCROLL_STEP_RIGHT);
  add_move_binding (binding_set, GDK_KP_Right, 0, GTK_SCROLL_STEP_RIGHT);

  add_move_binding (binding_set, GDK_Up, 0, GTK_SCROLL_STEP_UP);
  add_move_binding (binding_set, GDK_KP_Up, 0, GTK_SCROLL_STEP_UP);

  add_move_binding (binding_set, GDK_Down, 0, GTK_SCROLL_STEP_DOWN);
  add_move_binding (binding_set, GDK_KP_Down, 0, GTK_SCROLL_STEP_DOWN);
}

static void
gtk_resizer_init (GtkResizer *resizer)
{
  GtkResizerPrivate *priv;

  resizer->priv = priv = GTK_RESIZER_GET_PRIVATE (resizer);

  GTK_WIDGET_SET_FLAGS (resizer, GTK_CAN_FOCUS |  GTK_NO_WINDOW);

  priv->handle = NULL;
  priv->handle_pos.width = 5;
  priv->handle_pos.height = 5;
  priv->size_set = FALSE;
  priv->min_size = 1;
  priv->max_size = INT_MAX;
  priv->last_allocation = -1;
  priv->in_drag = FALSE;

  priv->handle_prelit = FALSE;
  priv->original_size = -1;
  
  priv->handle_pos.x = -1;
  priv->handle_pos.y = -1;

  priv->drag_pos = -1;
}

static void
gtk_resizer_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GtkResizer *resizer = GTK_RESIZER (object);

  switch (prop_id)
    {
    case PROP_SIZE:
      gtk_resizer_set_size (resizer, g_value_get_int (value));
      break;
    case PROP_SIZE_SET:
      resizer->priv->size_set = g_value_get_boolean (value);
      gtk_widget_queue_resize (GTK_WIDGET (resizer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_resizer_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GtkResizer *resizer = GTK_RESIZER (object);

  switch (prop_id)
    {
    case PROP_SIZE:
      g_value_set_int (value, resizer->priv->size);
      break;
    case PROP_SIZE_SET:
      g_value_set_boolean (value, resizer->priv->size_set);
      break;
    case PROP_MIN_SIZE:
      g_value_set_int (value, resizer->priv->min_size);
      break;
    case PROP_MAX_SIZE:
      g_value_set_int (value, resizer->priv->max_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_resizer_set_child_property (GtkContainer    *container,
                              GtkWidget       *child,
                              guint            property_id,
                              const GValue    *value,
                              GParamSpec      *pspec)
{
  GtkResizer *resizer = GTK_RESIZER (container);
  gboolean old_value, new_value;

  g_assert (child == gtk_bin_get_child (GTK_BIN (resizer)));

  new_value = g_value_get_boolean (value);
  switch (property_id)
    {
    case CHILD_PROP_SHRINK:
      old_value = resizer->priv->shrink;
      resizer->priv->shrink = new_value;
      if (old_value != new_value)
        gtk_widget_queue_resize (GTK_WIDGET (container));
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
gtk_resizer_get_child_property (GtkContainer *container,
                              GtkWidget    *child,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GtkResizer *resizer = GTK_RESIZER (container);

  g_assert (child == gtk_bin_get_child (GTK_BIN (resizer)));
  
  switch (property_id)
    {
    case CHILD_PROP_SHRINK:
      g_value_set_boolean (value, resizer->priv->shrink);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
gtk_resizer_realize (GtkWidget *widget)
{
  GtkBin *bin;
  GtkResizer *resizer;
  GtkResizerPrivate *priv;
  GdkWindowAttr attributes;
  gint attributes_mask;

  bin = GTK_BIN (widget);
  resizer = GTK_RESIZER (widget);
  priv = resizer->priv;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  widget->window = gtk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.x = priv->handle_pos.x;
  attributes.y = priv->handle_pos.y;
  attributes.width = priv->handle_pos.width;
  attributes.height = priv->handle_pos.height;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK |
                            GDK_POINTER_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;
  if (GTK_WIDGET_IS_SENSITIVE (widget))
    {
      attributes.cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                                      GTK_RESIZER_CURSOR);
      attributes_mask |= GDK_WA_CURSOR;
    }

  priv->handle = gdk_window_new (widget->window,
                                 &attributes, attributes_mask);
  gdk_window_set_user_data (priv->handle, resizer);
  if (attributes_mask & GDK_WA_CURSOR)
    gdk_cursor_unref (attributes.cursor);

  widget->style = gtk_style_attach (widget->style, widget->window);

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    gdk_window_show (priv->handle);
}

static void
gtk_resizer_unrealize (GtkWidget *widget)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;

  if (priv->handle)
    {
      gdk_window_set_user_data (priv->handle, NULL);
      gdk_window_destroy (priv->handle);
      priv->handle = NULL;
    }

  if (GTK_WIDGET_CLASS (gtk_resizer_parent_class)->unrealize)
    GTK_WIDGET_CLASS (gtk_resizer_parent_class)->unrealize (widget);
}

static void
gtk_resizer_map (GtkWidget *widget)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;

  gdk_window_show (priv->handle);

  GTK_WIDGET_CLASS (gtk_resizer_parent_class)->map (widget);
}

static void
gtk_resizer_unmap (GtkWidget *widget)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;

  gdk_window_hide (priv->handle);

  GTK_WIDGET_CLASS (gtk_resizer_parent_class)->unmap (widget);
}

static gboolean
gtk_resizer_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  GtkBin *bin = GTK_BIN (widget);
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget)
      && bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkStateType state;
      
      if (gtk_widget_is_focus (widget))
        state = GTK_STATE_SELECTED;
      else if (priv->handle_prelit)
        state = GTK_STATE_PRELIGHT;
      else
        state = GTK_WIDGET_STATE (widget);
      
      gtk_paint_resize_grip (widget->style, widget->window,
			     state, &priv->handle_pos, widget, "resizer",
			     GTK_RESIZER_EDGE,
			     priv->handle_pos.x, priv->handle_pos.y,
			     priv->handle_pos.width, priv->handle_pos.height);
    }

  /* Chain up to draw children */
  GTK_WIDGET_CLASS (gtk_resizer_parent_class)->expose_event (widget, event);
  
  return FALSE;
}

static void
update_drag (GtkResizer *resizer)
{
  GtkResizerPrivate *priv = resizer->priv;
  gint pos;
  gint size;
  
  gtk_widget_get_pointer (GTK_WIDGET (resizer), NULL, &pos);
  size = pos - priv->drag_pos;
  size = CLAMP (size, priv->min_size, priv->max_size);

  if (size != priv->size)
    gtk_resizer_set_size (resizer, size);
}

/* Why do we need the +/- 1 here?!?  */

static gboolean
gtk_resizer_enter_notify (GtkWidget        *widget,
                          GdkEventCrossing *event)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;
  
  if (priv->in_drag)
    update_drag (resizer);
  else
    {
      priv->handle_prelit = TRUE;
      gtk_widget_queue_draw_area (widget,
                                  priv->handle_pos.x,
                                  priv->handle_pos.y - 1,
                                  priv->handle_pos.width,
                                  priv->handle_pos.height + 1);
    }
  
  return TRUE;
}

static gboolean
gtk_resizer_leave_notify (GtkWidget        *widget,
                          GdkEventCrossing *event)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;
  
  if (priv->in_drag)
    update_drag (resizer);
  else
    {
      priv->handle_prelit = FALSE;
      gtk_widget_queue_draw_area (widget,
                                  priv->handle_pos.x,
                                  priv->handle_pos.y - 1,
                                  priv->handle_pos.width,
                                  priv->handle_pos.height + 1);
    }

  return TRUE;
}

static gboolean
gtk_resizer_focus (GtkWidget        *widget,
		    GtkDirectionType  direction)
{
  gboolean retval;

  /* This is a hack, but how can this be done without
   * excessive cut-and-paste from gtkcontainer.c?
   */

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
  retval = (* GTK_WIDGET_CLASS (gtk_resizer_parent_class)->focus) (widget, direction);
  GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);

  return retval;
}


static gboolean
gtk_resizer_button_press (GtkWidget      *widget,
			   GdkEventButton *event)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;

  if (!priv->in_drag &&
      (event->window == priv->handle) && (event->button == 1))
    {
      /* We need a server grab here, not gtk_grab_add(), since
       * we don't want to pass events on to the widget's children */
      if (gdk_pointer_grab (priv->handle, FALSE,
                            GDK_POINTER_MOTION_HINT_MASK
                            | GDK_BUTTON1_MOTION_MASK
                            | GDK_BUTTON_RELEASE_MASK
                            | GDK_ENTER_NOTIFY_MASK
                            | GDK_LEAVE_NOTIFY_MASK,
                            NULL, NULL,
                            event->time) != GDK_GRAB_SUCCESS)
        return FALSE;

      priv->in_drag = TRUE;
      priv->grab_time = event->time;
      priv->drag_pos = event->y + priv->handle_pos.y - priv->size;
      return TRUE;
    }

  return FALSE;
}

static gboolean
gtk_resizer_grab_broken (GtkWidget          *widget,
                         GdkEventGrabBroken *event)
{
  GtkResizer *resizer = GTK_RESIZER (widget);

  resizer->priv->in_drag = FALSE;
  resizer->priv->drag_pos = -1;
  resizer->priv->size_set = TRUE;

  return TRUE;
}

static void
stop_drag (GtkResizer *resizer)
{
  resizer->priv->in_drag = FALSE;
  resizer->priv->drag_pos = -1;
  resizer->priv->size_set = TRUE;
  gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (resizer)),
                              resizer->priv->grab_time);
}

static void
gtk_resizer_grab_notify (GtkWidget *widget,
			  gboolean   was_grabbed)
{
  GtkResizer *resizer = GTK_RESIZER (widget);

  if (!was_grabbed && resizer->priv->in_drag)
    stop_drag (resizer);
}

static void
gtk_resizer_state_changed (GtkWidget    *widget,
			    GtkStateType  previous_state)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  GtkResizerPrivate *priv = resizer->priv;

  if (GTK_WIDGET_REALIZED (resizer))
    {
      GdkCursor *cursor;

      if (GTK_WIDGET_IS_SENSITIVE (widget))
        cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                             GTK_RESIZER_CURSOR); 
      else
        cursor = NULL;

      gdk_window_set_cursor (priv->handle, cursor);

      if (cursor)
        gdk_cursor_unref (cursor);
    }
}

static gboolean
gtk_resizer_button_release (GtkWidget      *widget,
                            GdkEventButton *event)
{
  GtkResizer *resizer = GTK_RESIZER (widget);

  if (resizer->priv->in_drag && (event->button == 1))
    {
      stop_drag (resizer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gtk_resizer_motion (GtkWidget      *widget,
                    GdkEventMotion *event)
{
  GtkResizer *resizer = GTK_RESIZER (widget);
  
  if (resizer->priv->in_drag)
    {
      update_drag (resizer);
      return TRUE;
    }
  
  return FALSE;
}

static void
gtk_resizer_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  GtkResizer *resizer;
  GtkBin *bin;
  GtkResizerPrivate *priv;
  gint border_width;

  bin = GTK_BIN (widget);
  resizer = GTK_RESIZER (widget);
  priv = resizer->priv;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkRequisition child_requisition;
      gint handle_size;

      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width = child_requisition.width;
      if (priv->size_set)
	requisition->height = priv->size;
      else
	requisition->height = child_requisition.height;

      requisition->height = MIN (priv->max_size, requisition->height);
      requisition->height = MAX (priv->min_size, requisition->height);

      if (!priv->shrink)
	requisition->height = MAX (child_requisition.height, requisition->height);

      gtk_widget_style_get (widget, "handle-size", &handle_size, NULL);
      requisition->height += handle_size;
    }

  border_width = GTK_CONTAINER (widget)->border_width;
  requisition->width  += 2 * border_width;
  requisition->height += 2 * border_width;
}

static void
gtk_resizer_compute_size (GtkResizer *resizer,
                          gint      allocation,
                          gint      child_req)
{
  GtkBin *bin;
  GtkResizerPrivate *priv;
  gint old_size;
  gint old_min_size;
  gint old_max_size;
  
  bin = GTK_BIN (resizer);
  priv = resizer->priv;
  old_size = priv->size;
  old_min_size = priv->min_size;
  old_max_size = priv->max_size;

  priv->min_size = priv->shrink ? 0 : child_req;

  if (!priv->size_set)
    priv->size = child_req;

  priv->size = CLAMP (priv->size, priv->min_size, priv->max_size);

  /* gtk_widget_set_child_visible (bin->child, priv->size != 0); */

  g_object_freeze_notify (G_OBJECT (resizer));
  if (priv->size != old_size)
    g_object_notify (G_OBJECT (resizer), "size");
  if (priv->min_size != old_min_size)
    g_object_notify (G_OBJECT (resizer), "min-size");
  if (priv->max_size != old_max_size)
    g_object_notify (G_OBJECT (resizer), "max-size");
  g_object_thaw_notify (G_OBJECT (resizer));
}

static void
gtk_resizer_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GtkResizer *resizer;
  GtkBin *bin;
  GtkResizerPrivate *priv;
  gint border_width;

  resizer = GTK_RESIZER (widget);
  bin = GTK_BIN (widget);
  priv = resizer->priv;

  border_width = GTK_CONTAINER (widget)->border_width;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkRequisition child_requisition;
      GtkAllocation child_allocation;
      int handle_size;

      gtk_widget_style_get (widget, "handle-size", &handle_size, NULL);
      gtk_widget_get_child_requisition (bin->child, &child_requisition);
      gtk_resizer_compute_size (resizer,
				MAX (1, allocation->height
					- handle_size - 2 * border_width),
				child_requisition.height);

      priv->handle_pos.x = allocation->x + border_width;
      priv->handle_pos.y = allocation->y + priv->size + border_width;
      priv->handle_pos.width = MAX (1, (gint) allocation->width
					- 2 * border_width);
      priv->handle_pos.height = handle_size;

      if (GTK_WIDGET_REALIZED (widget))
        {
          if (GTK_WIDGET_MAPPED (widget))
	    gdk_window_show (priv->handle);

          gdk_window_move_resize (priv->handle,
			          priv->handle_pos.x,
			          priv->handle_pos.y,
			          priv->handle_pos.width,
			          priv->handle_pos.height);
	}

      child_allocation.x = allocation->x + border_width;
      child_allocation.y = allocation->y + border_width;
      child_allocation.width = MAX (1, (gint) allocation->width - 2 * border_width);
      child_allocation.height = MAX (1, priv->size);

      gtk_widget_size_allocate (bin->child, &child_allocation);
    }
  else
    {
      if (GTK_WIDGET_REALIZED (widget))
	gdk_window_hide (priv->handle);
    }
}

/**
 * gtk_resizer_get_size:
 * @resizer: a #GtkResizer widget
 * 
 * Obtains the size of the divider between the two panes.
 * 
 * Return value: size of the divider
 **/
gint
gtk_resizer_get_size (GtkResizer  *resizer)
{
  g_return_val_if_fail (GTK_IS_RESIZER (resizer), 0);

  return resizer->priv->size;
}

void
gtk_resizer_set_size (GtkResizer *resizer,
                        gint      size)
{
  GObject *object;
  
  g_return_if_fail (GTK_IS_RESIZER (resizer));

  object = G_OBJECT (resizer);
  
  if (size >= 0)
    {
      /* We don't clamp here - the assumption is that
       * if the total allocation changes at the same time
       * as the size, the size set is with reference
       * to the new total size. If only the size changes,
       * then clamping will occur in gtk_resizer_compute_size()
       */

      resizer->priv->size = size;
      resizer->priv->size_set = TRUE;
    }
  else
    {
      resizer->priv->size_set = FALSE;
    }

  g_object_freeze_notify (object);
  g_object_notify (object, "size");
  g_object_notify (object, "size-set");
  g_object_thaw_notify (object);

  gtk_widget_queue_resize (GTK_WIDGET (resizer));
}

static gboolean
gtk_resizer_move_handle (GtkResizer      *resizer,
                       GtkScrollType  scroll)
{
  if (gtk_widget_is_focus (GTK_WIDGET (resizer)))
    {
      gint old_size;
      gint new_size;
      gint increment;
      
      enum {
        SINGLE_STEP_SIZE = 1,
        PAGE_STEP_SIZE   = 75
      };
      
      new_size = old_size = gtk_resizer_get_size (resizer);
      increment = 0;
      
      switch (scroll)
        {
        case GTK_SCROLL_STEP_LEFT:
        case GTK_SCROLL_STEP_UP:
        case GTK_SCROLL_STEP_BACKWARD:
          increment = - SINGLE_STEP_SIZE;
          break;
          
        case GTK_SCROLL_STEP_RIGHT:
        case GTK_SCROLL_STEP_DOWN:
        case GTK_SCROLL_STEP_FORWARD:
          increment = SINGLE_STEP_SIZE;
          break;
          
        case GTK_SCROLL_PAGE_LEFT:
        case GTK_SCROLL_PAGE_UP:
        case GTK_SCROLL_PAGE_BACKWARD:
          increment = - PAGE_STEP_SIZE;
          break;
          
        case GTK_SCROLL_PAGE_RIGHT:
        case GTK_SCROLL_PAGE_DOWN:
        case GTK_SCROLL_PAGE_FORWARD:
          increment = PAGE_STEP_SIZE;
          break;

        case GTK_SCROLL_START:
          new_size = resizer->priv->min_size;
          break;
          
        case GTK_SCROLL_END:
          new_size = resizer->priv->max_size;
          break;

        default:
          break;
        }

      if (increment)
        new_size = old_size + increment;
      
      new_size = CLAMP (new_size, resizer->priv->min_size, resizer->priv->max_size); 
      if (old_size != new_size)
        gtk_resizer_set_size (resizer, new_size);

      return TRUE;
    }

  return FALSE;
}

static void
gtk_resizer_restore_focus (GtkResizer *resizer)
{
  if (gtk_widget_is_focus (GTK_WIDGET (resizer)))
    {
      GtkBin *bin = GTK_BIN (resizer);

      if (GTK_WIDGET_SENSITIVE (bin->child))
        {
          gtk_widget_grab_focus (bin->child);
        }
      else
        {
          /* the saved focus is somehow not available for focusing,
           * try
           *   1) tabbing into the resizer widget
           * if that didn't work,
           *   2) unset focus for the window if there is one
           */
          
          if (!gtk_widget_child_focus (GTK_WIDGET (resizer), GTK_DIR_TAB_FORWARD))
            {
              GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (resizer));
              
              if (GTK_IS_WINDOW (toplevel))
                gtk_window_set_focus (GTK_WINDOW (toplevel), NULL);
            }
        }
    }
}

static gboolean
gtk_resizer_accept_size (GtkResizer *resizer)
{
  if (gtk_widget_is_focus (GTK_WIDGET (resizer)))
    {
      resizer->priv->original_size = -1;
      gtk_resizer_restore_focus (resizer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gtk_resizer_cancel_size (GtkResizer *resizer)
{
  if (gtk_widget_is_focus (GTK_WIDGET (resizer)))
    {
      if (resizer->priv->original_size != -1)
        {
          gtk_resizer_set_size (resizer, resizer->priv->original_size);
          resizer->priv->original_size = -1;
        }

      gtk_resizer_restore_focus (resizer);
      return TRUE;
    }

  return FALSE;
}

static gboolean
gtk_resizer_toggle_handle_focus (GtkResizer *resizer)
{
  /* This function/signal has the wrong name. It is called when you
   * press Tab or Shift-Tab and what we do is act as if
   * the user pressed Return and then Tab or Shift-Tab
   */
  if (gtk_widget_is_focus (GTK_WIDGET (resizer)))
    gtk_resizer_accept_size (resizer);

  return FALSE;
}

/**
 * gtk_resizer_new:
 * 
 * Creates a new resizer.
 * 
 * Return value: a new #GtkResizer widget.
 *
 * Since: 2.4
 **/
GtkWidget *
gtk_resizer_new ()
{
  return g_object_new (GTK_TYPE_RESIZER, NULL);
}
