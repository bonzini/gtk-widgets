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

#ifndef __GTK_ELLIPSIS_H__
#define __GTK_ELLIPSIS_H__

#include <gtk/gtkbin.h>

G_BEGIN_DECLS

#define GTK_TYPE_ELLIPSIS            (gtk_ellipsis_get_type ())
#define GTK_ELLIPSIS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ELLIPSIS, GtkEllipsis))
#define GTK_ELLIPSIS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_ELLIPSIS, GtkEllipsisClass))
#define GTK_IS_ELLIPSIS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ELLIPSIS))
#define GTK_IS_ELLIPSIS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ELLIPSIS))
#define GTK_ELLIPSIS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ELLIPSIS, GtkEllipsisClass))

typedef struct _GtkEllipsis        GtkEllipsis;
typedef struct _GtkEllipsisClass   GtkEllipsisClass;
typedef struct _GtkEllipsisPrivate GtkEllipsisPrivate;

struct _GtkEllipsis
{
  GtkBin              bin;

  GtkEllipsisPrivate *priv;
};

struct _GtkEllipsisClass
{
  GtkBinClass    parent_class;

  /* Key binding signal; to get notification on the expansion
   * state connect to notify:expanded.
   */
  void        (* activate) (GtkEllipsis *ellipsis);
};

GType                 gtk_ellipsis_get_type          (void) G_GNUC_CONST;

GtkWidget            *gtk_ellipsis_new               (const gchar *label);
GtkWidget            *gtk_ellipsis_new_with_mnemonic (const gchar *label);

void                  gtk_ellipsis_set_expanded      (GtkEllipsis *ellipsis,
						      gboolean     expanded);
gboolean              gtk_ellipsis_get_expanded      (GtkEllipsis *ellipsis);

/* Spacing between the ellipsis/label and the child */
void                  gtk_ellipsis_set_spacing       (GtkEllipsis *ellipsis,
						      gint         spacing);
gint                  gtk_ellipsis_get_spacing       (GtkEllipsis *ellipsis);

void                  gtk_ellipsis_set_label         (GtkEllipsis *ellipsis,
						      const gchar *label);
G_CONST_RETURN gchar *gtk_ellipsis_get_label         (GtkEllipsis *ellipsis);

void		      gtk_ellipsis_set_line_wrap_mode (GtkEllipsis *ellipsis,
						       PangoWrapMode wrap_mode);
PangoWrapMode	      gtk_ellipsis_get_line_wrap_mode (GtkEllipsis *ellipsis);

void                  gtk_ellipsis_set_use_underline (GtkEllipsis *ellipsis,
						      gboolean     use_underline);
gboolean              gtk_ellipsis_get_use_underline (GtkEllipsis *ellipsis);

void                  gtk_ellipsis_set_use_markup    (GtkEllipsis *ellipsis,
						      gboolean    use_markup);
gboolean              gtk_ellipsis_get_use_markup    (GtkEllipsis *ellipsis);

void                  gtk_ellipsis_set_label_widget  (GtkEllipsis *ellipsis,
						      GtkWidget   *label_widget);
GtkWidget            *gtk_ellipsis_get_label_widget  (GtkEllipsis *ellipsis);

G_END_DECLS

#endif /* __GTK_ELLIPSIS_H__ */
