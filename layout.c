/* Demo for the GtkResizer and GtkEllipsis widgets.
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
#include "gtkstacklayout.h"

int main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *scrolled_window;
  GtkWidget *layout;
  GtkWidget *label;
  
  gtk_init (&argc, &argv);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Stack layout");
  g_signal_connect_after (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (window), scrolled_window);

  layout = gtk_stack_layout_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), layout);

  label = gtk_label_new ("This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text "
			 "This is a long text This is a long text");

  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_container_set_border_width (GTK_CONTAINER (layout), 4);
  gtk_container_add (GTK_CONTAINER (layout), label);

  gtk_window_set_default_size (GTK_WINDOW (window), 300, 40);
  gtk_widget_show_all (window);
  gtk_main ();
  return 0;
}
