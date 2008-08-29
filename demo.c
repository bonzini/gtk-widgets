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
#include "gtkresizer.h"
#include "gtkellipsis.h"

static gboolean
checkerboard_expose (GtkWidget	    *da,
		     GdkEventExpose *event,
		     gpointer	     data)
{
  gint i, j, xcount, ycount;
  GdkGC *gc1, *gc2;
  GdkColor color;
  
#define CHECK_SIZE 10
#define SPACING 2
  
  /* At the start of an expose handler, a clip region of event->area
   * is set on the window, and event->area has been cleared to the
   * widget's background color. The docs for
   * gdk_window_begin_paint_region() give more details on how this
   * works.
   */

  /* It would be a bit more efficient to keep these
   * GC's around instead of recreating on each expose, but
   * this is the lazy/slow way.
   */
  gc1 = gdk_gc_new (da->window);
  color.red = 30000;
  color.green = 0;
  color.blue = 30000;
  gdk_gc_set_rgb_fg_color (gc1, &color);

  gc2 = gdk_gc_new (da->window);
  color.red = 65535;
  color.green = 65535;
  color.blue = 65535;
  gdk_gc_set_rgb_fg_color (gc2, &color);
  
  xcount = 0;
  i = SPACING;
  while (i < da->allocation.width)
    {
      j = SPACING;
      ycount = xcount % 2; /* start with even/odd depending on row */
      while (j < da->allocation.height)
	{
	  GdkGC *gc;
	  
	  if (ycount % 2)
	    gc = gc1;
	  else
	    gc = gc2;

	  /* If we're outside event->area, this will do nothing.
	   * It might be mildly more efficient if we handled
	   * the clipping ourselves, but again we're feeling lazy.
	   */
	  gdk_draw_rectangle (da->window,
			      gc,
			      TRUE,
			      i, j,
			      CHECK_SIZE,
			      CHECK_SIZE);

	  j += CHECK_SIZE + SPACING;
	  ++ycount;
	}

      i += CHECK_SIZE + SPACING;
      ++xcount;
    }
  
  g_object_unref (gc1);
  g_object_unref (gc2);
  
  /* return TRUE because we've handled this event, so no
   * further processing is required.
   */
  return TRUE;
}

void
ellipsis_collapse (GtkButton *button, void *userdata)
{
  gtk_ellipsis_set_expanded (userdata, FALSE);
}

int main (int argc, char **argv)
{
  GtkWidget *ellipsis;
  GtkWidget *resizer;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *da;
  GtkWidget *window;
  GtkWidget *bbox;
  GtkWidget *button;
  
  gtk_init (&argc, &argv);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Resizer");
  g_signal_connect_after (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  ellipsis = gtk_ellipsis_new ("This is a long text This is a long text "
			       "This is a long text This is a long text "
			       "This is a long text This is a long text "
			       "This is a long text This is a long text");
  // gtk_widget_set_state (ellipsis, GTK_STATE_SELECTED);

  gtk_container_set_border_width (GTK_CONTAINER (ellipsis), 8);
  gtk_container_add (GTK_CONTAINER (window), ellipsis);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_add (GTK_CONTAINER (ellipsis), hbox);

  resizer = gtk_resizer_new ();
  gtk_box_pack_start (GTK_BOX (hbox), resizer, FALSE, TRUE, 0);

  /* First drawing area.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  da = gtk_drawing_area_new ();
  gtk_widget_set_size_request (da, 100, 100);

  gtk_container_add (GTK_CONTAINER (frame), da);
  gtk_container_add (GTK_CONTAINER (resizer), frame);

  gtk_container_child_set (GTK_CONTAINER (resizer),
			   frame, "shrink", TRUE, NULL);

  g_signal_connect (da, "expose_event",
		    G_CALLBACK (checkerboard_expose), NULL);

  /* Button.  */
  bbox = gtk_vbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_START);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
  g_signal_connect_after (button, "clicked",
                          G_CALLBACK (ellipsis_collapse), ellipsis);

  gtk_widget_show_all (window);
  gtk_main ();
  return 0;
}
