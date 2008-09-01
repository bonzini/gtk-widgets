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
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_LAYOUT_MANAGER_H__
#define __GTK_LAYOUT_MANAGER_H__


#include <gdk/gdk.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define GTK_TYPE_LAYOUT_MANAGER              (gtk_layout_manager_get_type ())
#define GTK_LAYOUT_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_LAYOUT_MANAGER, GtkLayoutManager))
#define GTK_LAYOUT_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_LAYOUT_MANAGER, GtkLayoutManagerClass))
#define GTK_IS_LAYOUT_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_LAYOUT_MANAGER))
#define GTK_IS_LAYOUT_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_LAYOUT_MANAGER))
#define GTK_LAYOUT_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_LAYOUT_MANAGER, GtkLayoutManagerClass))

#define GTK_IS_RESIZE_LAYOUT_MANAGER(widget) (GTK_IS_LAYOUT_MANAGER (widget) && ((GtkLayoutManager*) (widget))->resize_mode != GTK_RESIZE_PARENT)


typedef struct _GtkLayoutManager	GtkLayoutManager;
typedef struct _GtkLayoutManagerClass   GtkLayoutManagerClass;

typedef struct _GtkManagedLayout        GtkManagedLayout;
typedef struct _GtkManagedLayoutClass   GtkManagedLayoutClass;

struct _GtkLayoutManager
{
  GtkObject object;
  GtkLayoutManager *parent;
  GtkManagedLayout *root;
  GtkRequisition requisition;
  unsigned short border_width;
};

struct _GtkLayoutManagerClass
{
  GtkWidgetClass parent_class;

  void    (*add)       		(GtkLayoutManager   *layout_manager,
				 GtkLayoutManager   *submanager);
  void    (*remove)    		(GtkLayoutManager   *layout_manager,
				 GtkLayoutManager   *submanager);
  void    (*foreach_widget)	(GtkLayoutManager   *layout_manager,
				 GtkCallback	     callback,
				 gpointer	     callback_data);
  void    (*foreach)   		(GtkLayoutManager   *layout_manager,
				 GCallback	     callback,
				 gpointer	     callback_data);
  void    (*set_child_property) (GtkLayoutManager   *layout_manager,
				 GtkLayoutManager   *submanager,
				 guint       	     property_id,
				 const GValue	    *value,
				 GParamSpec  	    *pspec);
  void    (*get_child_property) (GtkLayoutManager   *layout_manager,
                                 GtkLayoutManager   *submanager,
				 guint    	     property_id,
				 GValue    	    *value,
				 GParamSpec	    *pspec);
  void    (*get_requisition)	(GtkLayoutManager   *layout_manager,
				 GtkRequisition	    *requisition);
  void    (*size_request)	(GtkLayoutManager   *layout_manager,
				 GtkRequisition	    *requisition);
  void    (*size_allocate)	(GtkLayoutManager   *layout_manager,
				 GtkAllocation	    *allocation);
};

/* Application-level methods */

GType   gtk_layout_manager_get_type		 (void) G_GNUC_CONST;

void    gtk_layout_manager_set_border_width	 (GtkLayoutManager	   *layout_manager,
					  	  guint		    border_width);

guint   gtk_layout_manager_get_border_width   (GtkLayoutManager     *layout_manager);
void    gtk_layout_manager_queue_resize  (GtkLayoutManager *layout_manager);

void    gtk_layout_manager_add		 (GtkLayoutManager	   *layout_manager,
					  GtkLayoutManager	   *submanager);

void    gtk_layout_manager_remove	 (GtkLayoutManager	   *layout_manager,
				  	  GtkLayoutManager	   *submanager);
void    gtk_layout_manager_get_requisition (GtkLayoutManager *layout_manager,
					    GtkRequisition   *requisition);
void    gtk_layout_manager_size_request  (GtkLayoutManager	   *layout_manager,
					  GtkRequisition	   *requisition);
void    gtk_layout_manager_size_allocate (GtkLayoutManager	   *layout_manager,
					  GtkAllocation		   *allocation);

GList*   gtk_layout_manager_get_children     (GtkLayoutManager       *layout_manager);


void         gtk_layout_manager_class_install_child_property (GtkLayoutManagerClass *cclass,
							      guint		    property_id,
							      GParamSpec	   *pspec);
GParamSpec*  gtk_layout_manager_class_find_child_property    (GObjectClass	   *cclass,
							      const gchar	   *property_name);
GParamSpec** gtk_layout_manager_class_list_child_properties  (GObjectClass	   *cclass,
							      guint		   *n_properties);
void         gtk_layout_manager_add_with_properties	     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *first_prop_name,
							      ...) G_GNUC_NULL_TERMINATED;
void         gtk_layout_manager_child_set		     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *first_prop_name,
							      ...) G_GNUC_NULL_TERMINATED;
void         gtk_layout_manager_child_get		     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *first_prop_name,
							      ...) G_GNUC_NULL_TERMINATED;
void         gtk_layout_manager_child_set_valist	     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *first_property_name,
							      va_list	    var_args);
void         gtk_layout_manager_child_get_valist	     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *first_property_name,
							      va_list	    var_args);
void	     gtk_layout_manager_child_set_property	     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *property_name,
							      const GValue	   *value);
void	     gtk_layout_manager_child_get_property	     (GtkLayoutManager	   *layout_manager,
							      GtkLayoutManager	   *submanager,
							      const gchar	   *property_name,
							      GValue		   *value);

#define GTK_LAYOUT_MANAGER_WARN_INVALID_CHILD_PROPERTY_ID(object, property_id, pspec) \
    G_OBJECT_WARN_INVALID_PSPEC ((object), "child property id", (property_id), (pspec))


void    gtk_layout_manager_foreach		     (GtkLayoutManager *layout_manager,
						      GCallback     callback,
						      gpointer	    callback_data);
void    gtk_layout_manager_foreach_widget	     (GtkLayoutManager *layout_manager,
						      GtkCallback   callback,
						      gpointer	    callback_data);

G_END_DECLS

#endif /* __GTK_LAYOUT_MANAGER_H__ */
