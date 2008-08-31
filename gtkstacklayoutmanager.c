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

#include <glib-2.0/glib.h>
#include <gtk/gtk.h>
#include "gtkstacklayout.h"
#include "gtklayoutmanager.h"
#include "gtkstacklayoutmanager.h"

#define I_(x)		(x)
#define P_(x)		(x)

static void gtk_stack_layout_manager_add (GtkLayoutManager *layout_manager,
	                                  GtkLayoutManager *submanager);
static void gtk_stack_layout_manager_remove (GtkLayoutManager *layout_manager,
	                                     GtkLayoutManager *submanager);
static void gtk_stack_layout_manager_finalize (GObject *object);
static void gtk_stack_layout_manager_foreach    (GtkLayoutManager   *layout_manager,
						 GCallback     callback,
						 gpointer        callback_data);
static void gtk_stack_layout_manager_foreach_widget    (GtkLayoutManager   *layout_manager,
						        GtkCallback     callback,
						        gpointer        callback_data);
static void gtk_stack_layout_manager_size_allocate (GtkLayoutManager *layout_manager,
	                                            GtkAllocation    *allocation);

G_DEFINE_TYPE (GtkStackLayoutManager, gtk_stack_layout_manager, GTK_TYPE_LAYOUT_MANAGER)

static void
gtk_stack_layout_manager_class_init (GtkStackLayoutManagerClass *class)
{
  GObjectClass *gobject_class;
  GtkLayoutManagerClass *layout_manager_class;

  gobject_class = G_OBJECT_CLASS (class);
  layout_manager_class = (GtkLayoutManagerClass*) class;

  gobject_class->finalize = gtk_stack_layout_manager_finalize;

  layout_manager_class->add = gtk_stack_layout_manager_add;
  layout_manager_class->remove = gtk_stack_layout_manager_remove;
  layout_manager_class->foreach = gtk_stack_layout_manager_foreach;
  layout_manager_class->foreach_widget = gtk_stack_layout_manager_foreach_widget;
  layout_manager_class->size_allocate = gtk_stack_layout_manager_size_allocate;
}

static void
gtk_stack_layout_manager_init (GtkStackLayoutManager *stack)
{
  stack->children = NULL;
  stack->p_tail = &stack->children;
}

static void
gtk_stack_layout_manager_finalize (GObject *object)
{
  GtkStackLayoutManager *stack = GTK_STACK_LAYOUT_MANAGER (object);

  while (stack->children)
    {
      g_object_unref (stack->children->data);
      stack->children = stack->children->next;
    }

  (* G_OBJECT_CLASS (gtk_stack_layout_manager_parent_class)->finalize) (object);
}

GtkLayoutManager *
gtk_stack_layout_manager_new (void)
{
  return g_object_new (GTK_TYPE_STACK_LAYOUT_MANAGER, NULL);
}

static void
gtk_stack_layout_manager_add (GtkLayoutManager *layout_manager,
			      GtkLayoutManager *submanager)
{
  GtkStackLayoutManager *stack = GTK_STACK_LAYOUT_MANAGER (layout_manager);
  GSList *child = g_slist_alloc ();

  /* The parent and root are held weakly.  */
  submanager->parent = layout_manager;
  submanager->root = layout_manager->root;

  child->data = submanager;
  child->next = NULL;
  *(stack->p_tail) = child;
  stack->p_tail = &child->next;
  
  g_object_ref_sink (submanager);
}

static void
gtk_stack_layout_manager_remove (GtkLayoutManager *layout_manager,
			         GtkLayoutManager *submanager)
{
  GtkStackLayoutManager *stack = GTK_STACK_LAYOUT_MANAGER (layout_manager);

  if (stack->children)
    stack->children = g_slist_remove (stack->children, submanager);

  g_object_ref_sink (submanager);
}

static void
gtk_stack_layout_manager_foreach_widget (GtkLayoutManager *layout_manager,
					 GtkCallback   callback,
					 gpointer      callback_data)
{
  GtkStackLayoutManager *stack = GTK_STACK_LAYOUT_MANAGER (layout_manager);
  GSList *child;

  g_return_if_fail (callback != NULL);

  for (child = stack->children; child; child = child->next)
    gtk_layout_manager_foreach_widget (child->data, callback, callback_data);
}

static void
gtk_stack_layout_manager_foreach (GtkLayoutManager *layout_manager,
				  GCallback   callback,
				  gpointer      callback_data)
{
  typedef void (*MyCallbackType) (GtkLayoutManager *, gpointer);

  GtkStackLayoutManager *stack = GTK_STACK_LAYOUT_MANAGER (layout_manager);
  MyCallbackType real_callback = (MyCallbackType) callback;
  GSList *child;

  g_return_if_fail (callback != NULL);

  for (child = stack->children; child; child = child->next)
    (*real_callback) (child->data, callback_data);
}

static void
gtk_stack_layout_manager_size_allocate (GtkLayoutManager *layout_manager,
	                                GtkAllocation    *allocation)
{
  GtkStackLayoutManager *stack = GTK_STACK_LAYOUT_MANAGER (layout_manager);
  GtkAllocation child_allocation;
  GSList *child;

  int border_width = layout_manager->border_width;

  for (child = stack->children; child; child = child->next)
    {
      GtkLayoutManager *submanager = GTK_LAYOUT_MANAGER (child->data);

      /* Allocate the subcomponent.  */
      child_allocation.x = allocation->x + border_width;
      child_allocation.y = allocation->y + border_width + allocation->height;
      child_allocation.width = 0;
      child_allocation.height = 0;
      gtk_layout_manager_size_allocate (submanager, &child_allocation);

      /* Tell the parent about our actual allocation.  */
      allocation->width = MAX (allocation->width, child_allocation.width);
      allocation->height += child_allocation.height + border_width;
   }
}
