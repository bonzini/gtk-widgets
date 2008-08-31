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

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include "gtklayoutmanager.h"
#include "gtkstacklayoutmarshal.h"
#include <gobject/gobjectnotifyqueue.c>
#include <gobject/gvaluecollector.h>

#define I_(x)		(x)
#define P_(x)		(x)

enum {
  ADD,
  REMOVE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BORDER_WIDTH
};

#define PARAM_SPEC_PARAM_ID(pspec)              ((pspec)->param_id)
#define PARAM_SPEC_SET_PARAM_ID(pspec, id)      ((pspec)->param_id = (id))


/* --- prototypes --- */
static void     gtk_layout_manager_base_class_init      (GtkLayoutManagerClass *klass);
static void     gtk_layout_manager_base_class_finalize  (GtkLayoutManagerClass *klass);
static void     gtk_layout_manager_class_init           (GtkLayoutManagerClass *klass);
static void     gtk_layout_manager_init                 (GtkLayoutManager      *layout_manager);
static void     gtk_layout_manager_finalize             (GObject	       *object);
static void     gtk_layout_manager_set_property         (GObject         *object,
						    guint            prop_id,
						    const GValue    *value,
						    GParamSpec      *pspec);
static void     gtk_layout_manager_get_property         (GObject         *object,
						    guint            prop_id,
						    GValue          *value,
						    GParamSpec      *pspec);
static void     gtk_layout_manager_add_unimplemented    (GtkLayoutManager      *layout_manager,
						    GtkLayoutManager	*submanager);
static void     gtk_layout_manager_remove_unimplemented (GtkLayoutManager      *layout_manager,
						    GtkLayoutManager	*submanager);
static void	gtk_layout_manager_children_size_request (GtkLayoutManager *submanager,
						          gpointer          client_data);
static void     gtk_layout_manager_children_callback (GtkLayoutManager *manager,
						      gpointer           client_data);
static void	gtk_layout_manager_get_requisition_impl (GtkLayoutManager *layout_manager,
		                                         GtkRequisition *requisition);
static void	gtk_layout_manager_size_request_impl (GtkLayoutManager *layout_manager,
						      GtkRequisition   *requisition);
static void	child_property_notify_dispatcher (GObject     *object,
		                                  guint        n_pspecs,
		                                  GParamSpec **pspecs);


/* --- variables --- */
static guint                 layout_manager_signals[LAST_SIGNAL] = { 0 };
static GtkObjectClass       *parent_class = NULL;
static GParamSpecPool       *_gtk_layout_manager_child_property_pool;
static GObjectNotifyContext *_gtk_layout_manager_child_property_notify_context;


/* --- functions --- */
GType
gtk_layout_manager_get_type (void)
{
  static GType layout_manager_type = 0;

  if (!layout_manager_type)
    {
      const GTypeInfo layout_manager_info =
      {
	sizeof (GtkLayoutManagerClass),
	(GBaseInitFunc) gtk_layout_manager_base_class_init,
	(GBaseFinalizeFunc) gtk_layout_manager_base_class_finalize,
	(GClassInitFunc) gtk_layout_manager_class_init,
	NULL        /* class_finalize */,
	NULL        /* class_data */,
	sizeof (GtkLayoutManager),
	0           /* n_preallocs */,
	(GInstanceInitFunc) gtk_layout_manager_init,
	NULL,       /* value_table */
      };

      layout_manager_type =
	g_type_register_static (GTK_TYPE_OBJECT, I_("GtkLayoutManager"), 
				&layout_manager_info, G_TYPE_FLAG_ABSTRACT);
    }

  return layout_manager_type;
}

static void
gtk_layout_manager_base_class_init (GtkLayoutManagerClass *class)
{
  /* reset instance specifc class fields that don't get inherited */
  class->set_child_property = NULL;
  class->get_child_property = NULL;
}

static void
gtk_layout_manager_base_class_finalize (GtkLayoutManagerClass *class)
{
  GList *list, *node;

  list = g_param_spec_pool_list_owned (_gtk_layout_manager_child_property_pool, G_OBJECT_CLASS_TYPE (class));
  for (node = list; node; node = node->next)
    {
      GParamSpec *pspec = node->data;

      g_param_spec_pool_remove (_gtk_layout_manager_child_property_pool, pspec);
      PARAM_SPEC_SET_PARAM_ID (pspec, 0);
      g_param_spec_unref (pspec);
    }
  g_list_free (list);
}

static void
gtk_layout_manager_class_init (GtkLayoutManagerClass *class)
{
  static GObjectNotifyContext cpn_context = { 0, NULL, NULL };

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = gtk_layout_manager_set_property;
  gobject_class->get_property = gtk_layout_manager_get_property;
  gobject_class->finalize = gtk_layout_manager_finalize;

  class->add = gtk_layout_manager_add_unimplemented;
  class->remove = gtk_layout_manager_remove_unimplemented;
  class->get_requisition = gtk_layout_manager_get_requisition_impl;
  class->size_request = gtk_layout_manager_size_request_impl;
  class->size_allocate = NULL;
  class->foreach = NULL;
  class->foreach_widget = NULL;

  _gtk_layout_manager_child_property_pool = g_param_spec_pool_new (TRUE);
  cpn_context.quark_notify_queue = g_quark_from_static_string ("GtkLayoutManager-child-property-notify-queue");
  cpn_context.dispatcher = child_property_notify_dispatcher;
  _gtk_layout_manager_child_property_notify_context = &cpn_context;

  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_WIDTH,
                                   g_param_spec_uint ("border-width",
                                                      P_("Border width"),
                                                      P_("The width of the empty border outside the layout_managers children"),
						      0,
						      G_MAXINT,
						      0,
                                                      G_PARAM_READWRITE));
  layout_manager_signals[ADD] =
    g_signal_new (I_("add"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkLayoutManagerClass, add),
		  NULL, NULL,
		  gtk_stacklayout_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GTK_TYPE_LAYOUT_MANAGER);
  layout_manager_signals[REMOVE] =
    g_signal_new (I_("remove"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkLayoutManagerClass, remove),
		  NULL, NULL,
		  gtk_stacklayout_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  GTK_TYPE_LAYOUT_MANAGER);
}


static void
child_property_notify_dispatcher (GObject     *object,
                                  guint        n_pspecs,
                                  GParamSpec **pspecs)
{
  GTK_WIDGET_GET_CLASS (object)->dispatch_child_properties_changed (GTK_WIDGET (object), n_pspecs, pspecs);
}

static void
gtk_layout_manager_finalize (GObject *object)
{
  GtkLayoutManager *layout_manager = GTK_LAYOUT_MANAGER (object);

  gtk_layout_manager_foreach (layout_manager,
			      (GCallback) g_object_unref, NULL);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/* --- GtkLayoutManager child property mechanism --- */
static inline void
layout_manager_get_child_property (GtkLayoutManager *layout_manager,
			      GtkLayoutManager    *child,
			      GParamSpec   *pspec,
			      GValue       *value)
{
  GtkLayoutManagerClass *class = g_type_class_peek (pspec->owner_type);
  
  class->get_child_property (layout_manager, child, PARAM_SPEC_PARAM_ID (pspec), value, pspec);
}

static inline void
layout_manager_set_child_property (GtkLayoutManager   *layout_manager,
			           GtkLayoutManager	      *child,
			           GParamSpec         *pspec,
			           const GValue       *value,
			           GObjectNotifyQueue *nqueue)
{
  GValue tmp_value; // = { 0, };
  GtkLayoutManagerClass *class = g_type_class_peek (pspec->owner_type);

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set child property `%s' of type `%s' from value of type `%s'",
	       pspec->name,
	       g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
	       G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      gchar *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type `%s' is invalid for property `%s' of type `%s'",
		 contents,
		 G_VALUE_TYPE_NAME (value),
		 pspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      g_free (contents);
    }
  else
    {
      class->set_child_property (layout_manager, child, PARAM_SPEC_PARAM_ID (pspec), &tmp_value, pspec);
      g_object_notify_queue_add (G_OBJECT (child), nqueue, pspec);
    }
  g_value_unset (&tmp_value);
}

/**
 * gtk_layout_manager_child_get_valist:
 * @layout_manager: a #GtkLayoutManager
 * @child: a submanager which is a child of @layout_manager
 * @first_property_name: the name of the first property to get
 * @var_args: a %NULL-terminated list of property names and #GValue*, 
 *           starting with @first_prop_name.
 * 
 * Gets the values of one or more child properties for @child and @layout_manager.
 **/
void
gtk_layout_manager_child_get_valist (GtkLayoutManager *layout_manager,
				GtkLayoutManager    *child,
				const gchar  *first_property_name,
				va_list       var_args)
{
  const gchar *name;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (child));
  g_return_if_fail (child->parent == GTK_LAYOUT_MANAGER (layout_manager));

  g_object_ref (layout_manager);
  g_object_ref (child);

  name = first_property_name;
  while (name)
    {
      GValue value; // = { 0, };
      GParamSpec *pspec;
      gchar *error;

      pspec = g_param_spec_pool_lookup (_gtk_layout_manager_child_property_pool,
					name,
					G_OBJECT_TYPE (layout_manager),
					TRUE);
      if (!pspec)
	{
	  g_warning ("%s: layout_manager class `%s' has no child property named `%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (layout_manager),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: child property `%s' of layout_manager class `%s' is not readable",
		     G_STRLOC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (layout_manager));
	  break;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      layout_manager_get_child_property (layout_manager, child, pspec, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  g_value_unset (&value);
	  break;
	}
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }

  g_object_unref (child);
  g_object_unref (layout_manager);
}

/**
 * gtk_layout_manager_child_get_property:
 * @layout_manager: a #GtkLayoutManager
 * @child: a submanager which is a child of @layout_manager
 * @property_name: the name of the property to get
 * @value: a location to return the value
 * 
 * Gets the value of a child property for @child and @layout_manager.
 **/
void
gtk_layout_manager_child_get_property (GtkLayoutManager *layout_manager,
				  GtkLayoutManager    *child,
				  const gchar  *property_name,
				  GValue       *value)
{
  GParamSpec *pspec;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (child));
  g_return_if_fail (child->parent == GTK_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (layout_manager);
  g_object_ref (child);
  pspec = g_param_spec_pool_lookup (_gtk_layout_manager_child_property_pool, property_name,
				    G_OBJECT_TYPE (layout_manager), TRUE);
  if (!pspec)
    g_warning ("%s: layout_manager class `%s' has no child property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (layout_manager),
	       property_name);
  else if (!(pspec->flags & G_PARAM_READABLE))
    g_warning ("%s: child property `%s' of layout_manager class `%s' is not readable",
	       G_STRLOC,
	       pspec->name,
	       G_OBJECT_TYPE_NAME (layout_manager));
  else
    {
      GValue *prop_value, tmp_value; // = { 0, };

      /* auto-conversion of the callers value type
       */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
	{
	  g_value_reset (value);
	  prop_value = value;
	}
      else if (!g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
	{
	  g_warning ("can't retrieve child property `%s' of type `%s' as value of type `%s'",
		     pspec->name,
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		     G_VALUE_TYPE_NAME (value));
	  g_object_unref (child);
	  g_object_unref (layout_manager);
	  return;
	}
      else
	{
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  prop_value = &tmp_value;
	}
      layout_manager_get_child_property (layout_manager, child, pspec, prop_value);
      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }
  g_object_unref (child);
  g_object_unref (layout_manager);
}

/**
 * gtk_layout_manager_child_set_valist:
 * @layout_manager: a #GtkLayoutManager
 * @child: a submanager which is a child of @layout_manager
 * @first_property_name: the name of the first property to set
 * @var_args: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 * 
 * Sets one or more child properties for @child and @layout_manager.
 **/
void
gtk_layout_manager_child_set_valist (GtkLayoutManager *layout_manager,
				GtkLayoutManager    *child,
				const gchar  *first_property_name,
				va_list       var_args)
{
  GObjectNotifyQueue *nqueue;
  const gchar *name;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (child));
  g_return_if_fail (child->parent == GTK_LAYOUT_MANAGER (layout_manager));

  g_object_ref (layout_manager);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _gtk_layout_manager_child_property_notify_context);
  name = first_property_name;
  while (name)
    {
      GValue value; // = { 0, };
      gchar *error = NULL;
      GParamSpec *pspec = g_param_spec_pool_lookup (_gtk_layout_manager_child_property_pool,
						    name,
						    G_OBJECT_TYPE (layout_manager),
						    TRUE);
      if (!pspec)
	{
	  g_warning ("%s: layout_manager class `%s' has no child property named `%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (layout_manager),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: child property `%s' of layout_manager class `%s' is not writable",
		     G_STRLOC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (layout_manager));
	  break;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      layout_manager_set_child_property (layout_manager, child, pspec, &value, nqueue);
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);

  g_object_unref (layout_manager);
  g_object_unref (child);
}

/**
 * gtk_layout_manager_child_set_property:
 * @layout_manager: a #GtkLayoutManager
 * @child: a submanager which is a child of @layout_manager
 * @property_name: the name of the property to set
 * @value: the value to set the property to
 * 
 * Sets a child property for @child and @layout_manager.
 **/
void
gtk_layout_manager_child_set_property (GtkLayoutManager *layout_manager,
				  GtkLayoutManager    *child,
				  const gchar  *property_name,
				  const GValue *value)
{
  GObjectNotifyQueue *nqueue;
  GParamSpec *pspec;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (child));
  g_return_if_fail (child->parent == GTK_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (layout_manager);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _gtk_layout_manager_child_property_notify_context);
  pspec = g_param_spec_pool_lookup (_gtk_layout_manager_child_property_pool, property_name,
				    G_OBJECT_TYPE (layout_manager), TRUE);
  if (!pspec)
    g_warning ("%s: layout_manager class `%s' has no child property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (layout_manager),
	       property_name);
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    g_warning ("%s: child property `%s' of layout_manager class `%s' is not writable",
	       G_STRLOC,
	       pspec->name,
	       G_OBJECT_TYPE_NAME (layout_manager));
  else
    {
      layout_manager_set_child_property (layout_manager, child, pspec, value, nqueue);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);
  g_object_unref (layout_manager);
  g_object_unref (child);
}

/**
 * gtk_layout_manager_add_with_properties:
 * @layout_manager: a #GtkLayoutManager 
 * @submanager: a submanager to be placed inside @layout_manager 
 * @first_prop_name: the name of the first child property to set 
 * @Varargs: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 * 
 * Adds @submanager to @layout_manager, setting child properties at the same time.
 * See gtk_layout_manager_add() and gtk_layout_manager_child_set() for more details.
 **/
void
gtk_layout_manager_add_with_properties (GtkLayoutManager *layout_manager,
				   GtkLayoutManager    *submanager,
				   const gchar  *first_prop_name,
				   ...)
{
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (submanager));
  g_return_if_fail (submanager->parent == NULL);

  g_object_ref (layout_manager);
  g_object_ref (submanager);
  if (layout_manager->root)
    gtk_widget_freeze_child_notify (GTK_WIDGET (layout_manager->root));

  g_signal_emit (layout_manager, layout_manager_signals[ADD], 0, submanager);
  if (submanager->parent)
    {
      va_list var_args;

      va_start (var_args, first_prop_name);
      gtk_layout_manager_child_set_valist (layout_manager, submanager, first_prop_name, var_args);
      va_end (var_args);
    }

  if (layout_manager->root)
    gtk_widget_thaw_child_notify (GTK_WIDGET (layout_manager->root));
  g_object_unref (submanager);
  g_object_unref (layout_manager);
}

/**
 * gtk_layout_manager_child_set:
 * @layout_manager: a #GtkLayoutManager
 * @child: a submanager which is a child of @layout_manager
 * @first_prop_name: the name of the first property to set
 * @Varargs: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 * 
 * Sets one or more child properties for @child and @layout_manager.
 **/
void
gtk_layout_manager_child_set (GtkLayoutManager      *layout_manager,
			 GtkLayoutManager         *child,
			 const gchar       *first_prop_name,
			 ...)
{
  va_list var_args;
  
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (child));
  g_return_if_fail (child->parent == GTK_LAYOUT_MANAGER (layout_manager));

  va_start (var_args, first_prop_name);
  gtk_layout_manager_child_set_valist (layout_manager, child, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * gtk_layout_manager_child_get:
 * @layout_manager: a #GtkLayoutManager
 * @child: a submanager which is a child of @layout_manager
 * @first_prop_name: the name of the first property to get
 * @Varargs: a %NULL-terminated list of property names and #GValue*, 
 *           starting with @first_prop_name
 * 
 * Gets the values of one or more child properties for @child and @layout_manager.
 **/
void
gtk_layout_manager_child_get (GtkLayoutManager      *layout_manager,
			 GtkLayoutManager         *child,
			 const gchar       *first_prop_name,
			 ...)
{
  va_list var_args;
  
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (child));
  g_return_if_fail (child->parent == GTK_LAYOUT_MANAGER (layout_manager));

  va_start (var_args, first_prop_name);
  gtk_layout_manager_child_get_valist (layout_manager, child, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * gtk_layout_manager_class_install_child_property:
 * @cclass: a #GtkLayoutManagerClass
 * @property_id: the id for the property
 * @pspec: the #GParamSpec for the property
 * 
 * Installs a child property on a layout_manager class. 
 **/
void
gtk_layout_manager_class_install_child_property (GtkLayoutManagerClass *cclass,
					    guint              property_id,
					    GParamSpec        *pspec)
{
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER_CLASS (cclass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (pspec->flags & G_PARAM_WRITABLE)
    g_return_if_fail (cclass->set_child_property != NULL);
  if (pspec->flags & G_PARAM_READABLE)
    g_return_if_fail (cclass->get_child_property != NULL);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);  /* paranoid */
  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    g_return_if_fail ((pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)) == 0);

  if (g_param_spec_pool_lookup (_gtk_layout_manager_child_property_pool, pspec->name, G_OBJECT_CLASS_TYPE (cclass), FALSE))
    {
      g_warning (G_STRLOC ": class `%s' already contains a child property named `%s'",
		 G_OBJECT_CLASS_NAME (cclass),
		 pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  g_param_spec_pool_insert (_gtk_layout_manager_child_property_pool, pspec, G_OBJECT_CLASS_TYPE (cclass));
}

/**
 * gtk_layout_manager_class_find_child_property:
 * @cclass: a #GtkLayoutManagerClass
 * @property_name: the name of the child property to find
 * @returns: the #GParamSpec of the child property or %NULL if @class has no
 *   child property with that name.
 *
 * Finds a child property of a layout_manager class by name.
 */
GParamSpec*
gtk_layout_manager_class_find_child_property (GObjectClass *cclass,
					 const gchar  *property_name)
{
  g_return_val_if_fail (GTK_IS_LAYOUT_MANAGER_CLASS (cclass), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (_gtk_layout_manager_child_property_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (cclass),
				   TRUE);
}

/**
 * gtk_layout_manager_class_list_child_properties:
 * @cclass: a #GtkLayoutManagerClass
 * @n_properties: location to return the number of child properties found
 * @returns: a newly allocated %NULL-terminated array of #GParamSpec*. 
 *           The array must be freed with g_free().
 *
 * Returns all child properties of a layout_manager class.
 */
GParamSpec**
gtk_layout_manager_class_list_child_properties (GObjectClass *cclass,
					   guint        *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (GTK_IS_LAYOUT_MANAGER_CLASS (cclass), NULL);

  pspecs = g_param_spec_pool_list (_gtk_layout_manager_child_property_pool,
				   G_OBJECT_CLASS_TYPE (cclass),
				   &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}

static void
gtk_layout_manager_add_unimplemented (GtkLayoutManager     *layout_manager,
				      GtkLayoutManager        *submanager)
{
  g_warning ("GtkLayoutManagerClass::add not implemented for `%s'", g_type_name (G_TYPE_FROM_INSTANCE (layout_manager)));
}

static void
gtk_layout_manager_remove_unimplemented (GtkLayoutManager     *layout_manager,
				         GtkLayoutManager        *submanager)
{
  g_warning ("GtkLayoutManagerClass::remove not implemented for `%s'", g_type_name (G_TYPE_FROM_INSTANCE (layout_manager)));
}

static void
gtk_layout_manager_init (GtkLayoutManager *layout_manager)
{
  layout_manager->border_width = 0;
}

static void
gtk_layout_manager_set_property (GObject         *object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
  GtkLayoutManager *layout_manager = GTK_LAYOUT_MANAGER (object);

  switch (prop_id)
    {
    case PROP_BORDER_WIDTH:
      gtk_layout_manager_set_border_width (layout_manager, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_layout_manager_get_property (GObject         *object,
			    guint            prop_id,
			    GValue          *value,
			    GParamSpec      *pspec)
{
  GtkLayoutManager *layout_manager = GTK_LAYOUT_MANAGER (object);
  
  switch (prop_id)
    {
    case PROP_BORDER_WIDTH:
      g_value_set_uint (value, layout_manager->border_width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * gtk_layout_manager_set_border_width:
 * @layout_manager: a #GtkLayoutManager
 * @border_width: amount of blank space to leave <emphasis>outside</emphasis> 
 *   the layout_manager. Valid values are in the range 0-65535 pixels.
 *
 * Sets the border width of the layout_manager.
 *
 * The border width of a layout_manager is the amount of space to leave
 * around the outside of the layout_manager. The only exception to this is
 * #GtkWindow; because toplevel windows can't leave space outside,
 * they leave the space inside. The border is added on all sides of
 * the layout_manager. To add space to only one side, one approach is to
 * create a #GtkAlignment submanager, call gtk_widget_set_size_request()
 * to give it a size, and place it on the side of the layout_manager as
 * a spacer.
 **/
void
gtk_layout_manager_set_border_width (GtkLayoutManager *layout_manager,
				guint         border_width)
{
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));

  if (layout_manager->border_width != border_width)
    {
      layout_manager->border_width = border_width;
      g_object_notify (G_OBJECT (layout_manager), "border-width");
      
      gtk_layout_manager_queue_resize (layout_manager);
    }
}

/**
 * gtk_layout_manager_queue_resize:
 * @layout_manager: a #GtkLayoutManager
 * 
 * Queues a resize of the GtkStackLayout that is hosting @layout_manager.
 **/
void
gtk_layout_manager_queue_resize (GtkLayoutManager *layout_manager)
{
  if (layout_manager->root && GTK_WIDGET_REALIZED (layout_manager->root))
    gtk_widget_queue_resize (GTK_WIDGET (layout_manager->root));
}

/**
 * gtk_layout_manager_get_border_width:
 * @layout_manager: a #GtkLayoutManager
 * 
 * Retrieves the border width of the layout_manager. See
 * gtk_layout_manager_set_border_width().
 *
 * Return value: the current border width
 **/
guint
gtk_layout_manager_get_border_width (GtkLayoutManager *layout_manager)
{
  g_return_val_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager), 0);

  return layout_manager->border_width;
}

/**
 * gtk_layout_manager_add:
 * @layout_manager: a #GtkLayoutManager
 * @submanager: a submanager to be placed inside @layout_manager
 * 
 * Adds @submanager to @layout_manager. Typically used for simple layout_managers
 * such as #GtkWindow, #GtkFrame, or #GtkButton; for more complicated
 * layout layout_managers such as #GtkBox or #GtkTable, this function will
 * pick default packing parameters that may not be correct.  So
 * consider functions such as gtk_box_pack_start() and
 * gtk_table_attach() as an alternative to gtk_layout_manager_add() in
 * those cases. A submanager may be added to only one layout_manager at a time;
 * you can't place the same submanager inside two different layout_managers.
 **/
void
gtk_layout_manager_add (GtkLayoutManager *layout_manager,
		   GtkLayoutManager    *submanager)
{
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (submanager));

  if (submanager->parent != NULL)
    {
      g_warning ("Attempting to add a submanager with type %s to a layout_manager of "
                 "type %s, but the submanager is already inside a layout_manager of type %s, "
                 "the GTK+ FAQ at http://www.gtk.org/faq/ explains how to reparent a submanager.",
                 g_type_name (G_OBJECT_TYPE (submanager)),
                 g_type_name (G_OBJECT_TYPE (layout_manager)),
                 g_type_name (G_OBJECT_TYPE (submanager->parent)));
      return;
    }

  g_signal_emit (layout_manager, layout_manager_signals[ADD], 0, submanager);
}

/**
 * gtk_layout_manager_remove:
 * @layout_manager: a #GtkLayoutManager
 * @submanager: a current child of @layout_manager
 * 
 * Removes @submanager from @layout_manager. @submanager must be inside @layout_manager.
 * Note that @layout_manager will own a reference to @submanager, and that this
 * may be the last reference held; so removing a submanager from its
 * layout_manager can destroy that submanager. If you want to use @submanager
 * again, you need to add a reference to it while it's not inside
 * a layout_manager, using g_object_ref(). If you don't want to use @submanager
 * again it's usually more efficient to simply destroy it directly
 * using gtk_widget_destroy() since this will remove it from the
 * layout_manager and help break any circular reference count cycles.
 **/
void
gtk_layout_manager_remove (GtkLayoutManager *layout_manager,
		      GtkLayoutManager    *submanager)
{
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (submanager));

  g_signal_emit (layout_manager, layout_manager_signals[REMOVE], 0, submanager);
}

/**
 * gtk_layout_manager_foreach_widget:
 * @layout_manager: a #GtkLayoutManager
 * @callback: a callback
 * @callback_data: callback user data
 * 
 * Invokes @callback on each child of @layout_manager, including nested
 * widgets reachable through other layout managers.
 **/
void
gtk_layout_manager_foreach_widget (GtkLayoutManager *layout_manager,
		      GtkCallback   callback,
		      gpointer      callback_data)
{
  GtkLayoutManagerClass *class;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (callback != NULL);

  class = GTK_LAYOUT_MANAGER_GET_CLASS (layout_manager);

  if (class->foreach_widget)
    class->foreach_widget (layout_manager, callback, callback_data);
}

/**
 * gtk_layout_manager_foreach:
 * @layout_manager: a #GtkLayoutManager
 * @callback: a callback
 * @callback_data: callback user data
 * 
 * Invokes @callback on each layout manager directly included in
 * @layout_manager.
 **/
void
gtk_layout_manager_foreach (GtkLayoutManager *layout_manager,
		      GCallback   callback,
		      gpointer      callback_data)
{
  GtkLayoutManagerClass *class;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (callback != NULL);

  class = GTK_LAYOUT_MANAGER_GET_CLASS (layout_manager);

  if (class->foreach)
    class->foreach (layout_manager, callback, callback_data);
}



static void
gtk_layout_manager_get_requisition_impl (GtkLayoutManager *layout_manager,
				         GtkRequisition   *requisition)
{
  *requisition = layout_manager->requisition;
}

static void
gtk_layout_manager_size_request_impl (GtkLayoutManager *layout_manager,
				      GtkRequisition   *requisition)
{
  requisition->width = 0;
  requisition->height = 0;
  gtk_layout_manager_foreach (layout_manager,
			      (GCallback) gtk_layout_manager_children_size_request,
			      requisition);

  requisition->width += 2 * layout_manager->border_width;
  requisition->height += 2 * layout_manager->border_width;
  layout_manager->requisition = *requisition;
}

static void
gtk_layout_manager_children_size_request (GtkLayoutManager *submanager,
				          gpointer          client_data)
{
  GtkRequisition *requisition = client_data;
  GtkRequisition child_requisition;

  gtk_layout_manager_size_request (submanager, &child_requisition);
  requisition->width = MAX (child_requisition.width, requisition->width);
  requisition->height = MAX (child_requisition.height, requisition->height);
}

/**
 * gtk_layout_manager_get_requisition:
 * @layout_manager: a #GtkLayoutManager
 * @requisition: place to fill in the retrieved width and height requirements
 * of the layout manager subtree.
 **/
void
gtk_layout_manager_get_requisition (GtkLayoutManager *layout_manager,
				    GtkRequisition   *requisition)
{
  GtkLayoutManagerClass *class;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (requisition != NULL);

  class = GTK_LAYOUT_MANAGER_GET_CLASS (layout_manager);

  if (class->get_requisition)
    class->get_requisition (layout_manager, requisition);
}

/**
 * gtk_layout_manager_size_request:
 * @layout_manager: a #GtkLayoutManager
 * @requisition: place to fill in the width and height requirements
 * of the layout manager subtree.
 **/
void
gtk_layout_manager_size_request (GtkLayoutManager *layout_manager,
				 GtkRequisition	   *requisition)
{
  GtkLayoutManagerClass *class;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (requisition != NULL);

  class = GTK_LAYOUT_MANAGER_GET_CLASS (layout_manager);

  if (class->size_request)
    class->size_request (layout_manager, requisition);
}

/**
 * gtk_layout_manager_size_allocate:
 * @layout_manager: a #GtkLayoutManager
 * @allocation: starting point and width to allocate children in.
 * 
 **/
void
gtk_layout_manager_size_allocate (GtkLayoutManager *layout_manager,
				  GtkAllocation	   *allocation)
{
  GtkLayoutManagerClass *class;

  g_return_if_fail (GTK_IS_LAYOUT_MANAGER (layout_manager));
  g_return_if_fail (allocation != NULL);
  g_return_if_fail (allocation->width == 0);
  g_return_if_fail (allocation->height == 0);

  class = GTK_LAYOUT_MANAGER_GET_CLASS (layout_manager);

  if (class->size_allocate)
    class->size_allocate (layout_manager, allocation);
}


/**
 * gtk_layout_manager_get_children:
 * @layout_manager: a #GtkLayoutManager
 * 
 * Returns the layout manager's child layout manager.
 *
 * Return value: a newly-allocated list of the layout_manager's leaf widgets.
 **/
GList*
gtk_layout_manager_get_children (GtkLayoutManager *layout_manager)
{
  GList *children = NULL;

  gtk_layout_manager_foreach (layout_manager,
			      (GCallback) gtk_layout_manager_children_callback,
			      &children);

  return g_list_reverse (children);
}

static void
gtk_layout_manager_children_callback (GtkLayoutManager *manager,
				      gpointer   client_data)
{
  GList **children;

  children = (GList**) client_data;
  *children = g_list_prepend (*children, manager);
}

