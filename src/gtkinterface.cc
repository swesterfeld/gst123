/* GST123 - GStreamer based command line media player
 * Copyright (C) 2010 Stefan Westerfeld
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gtkinterface.h"

#include <gtk/gtk.h>
#include <X11/Xlib.h>

void
GtkInterface::init (int *argc, char ***argv)
{
  if (XOpenDisplay (NULL))
    {
      gtk_init (argc, argv);
      gtk_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_widget_show_all (gtk_window);
    }
  else
    {
      gtk_window = NULL;
    }
}

void
GtkInterface::toggle_fullscreen()
{
  gboolean isFullscreen = (gdk_window_get_state (GDK_WINDOW (gtk_window->window)) == GDK_WINDOW_STATE_FULLSCREEN);
  if (isFullscreen)
    gtk_window_unfullscreen (GTK_WINDOW (gtk_window));
  else
    gtk_window_fullscreen (GTK_WINDOW (gtk_window));
}

bool
GtkInterface::init_ok()
{
  return gtk_window != NULL;
}

GtkWidget*
GtkInterface::window()
{
  return gtk_window;
}
