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

#ifndef __GTK_RESIZER_H__
#define __GTK_RESIZER_H__

#include <gtk/gtkbin.h>

G_BEGIN_DECLS

#define GTK_TYPE_RESIZER            (gtk_resizer_get_type ())
#define GTK_RESIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_RESIZER, GtkResizer))
#define GTK_RESIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_RESIZER, GtkResizerClass))
#define GTK_IS_RESIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_RESIZER))
#define GTK_IS_RESIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_RESIZER))
#define GTK_RESIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_RESIZER, GtkResizerClass))

typedef struct _GtkResizer        GtkResizer;
typedef struct _GtkResizerClass   GtkResizerClass;
typedef struct _GtkResizerPrivate GtkResizerPrivate;

struct _GtkResizer
{
  GtkBin              bin;

  GtkResizerPrivate *priv;
};

struct _GtkResizerClass
{
  GtkBinClass    parent_class;

  gboolean (* toggle_handle_focus) (GtkResizer      *resizer);
  gboolean (* move_handle)         (GtkResizer      *resizer,
                                    GtkScrollType  scroll);
  gboolean (* accept_size)	   (GtkResizer      *resizer);
  gboolean (* cancel_size)	   (GtkResizer      *resizer);
};

GType                 gtk_resizer_get_type   (void) G_GNUC_CONST;

GtkWidget            *gtk_resizer_new        (void);

gint		      gtk_resizer_get_size   (GtkResizer *resizer);
void		      gtk_resizer_set_size   (GtkResizer *resizer,
					      gint position);

G_END_DECLS

#endif /* __GTK_RESIZER_H__ */
