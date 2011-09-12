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
#include <gdk/gdkkeysyms.h>

using std::string;

static gboolean
key_press_event_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  GtkInterface *gtk_interface = static_cast<GtkInterface *> (data);

  return gtk_interface->handle_keypress_event (event);
}

static gboolean
motion_notify_event_cb (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
  GtkInterface *gtk_interface = static_cast<GtkInterface *> (data);

  return gtk_interface->handle_motion_notify_event (event);
}

static gboolean
timeout_cb (gpointer data)
{
  GtkInterface *gtk_interface = static_cast<GtkInterface *> (data);

  return gtk_interface->handle_timeout();
}

static gboolean
close_cb (GtkWidget *widget,
          GdkEvent  *event,
          gpointer   data)
{
  GtkInterface *gtk_interface = static_cast<GtkInterface *> (data);

  return gtk_interface->handle_close();
}

bool
GtkInterface::have_x11_display()
{
  static Display *display = NULL;

  if (!display)
    display = XOpenDisplay (NULL);   // this should work if and only if we do have an X11 server we can use

  return display != NULL;
}

void
GtkInterface::init (int *argc, char ***argv, KeyHandler *handler)
{
  key_handler = handler;

  if (have_x11_display())
    {
      gtk_init (argc, argv);
      gtk_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      g_signal_connect (G_OBJECT (gtk_window), "key-press-event", G_CALLBACK (key_press_event_cb), this);
      g_signal_connect (G_OBJECT (gtk_window), "motion-notify-event", G_CALLBACK (motion_notify_event_cb), this);
      g_signal_connect (G_OBJECT (gtk_window), "delete-event", G_CALLBACK  (close_cb), this);
      g_object_set (G_OBJECT (gtk_window), "events", GDK_POINTER_MOTION_MASK, NULL);

      // make background black
      GdkColor color;
      gdk_color_parse ("black", &color);
      gtk_widget_modify_bg (gtk_window, GTK_STATE_NORMAL, &color);

      visible_cursor = NULL;
      invisible_cursor = gdk_cursor_new (GDK_BLANK_CURSOR);

      cursor_timeout = 3;
      g_timeout_add (500, (GSourceFunc) timeout_cb, this);
    }
  else
    {
      gtk_window = NULL;
    }
  gtk_window_visible = false;

  /* initialize map from Gdk keysyms to KeyHandler codes */
  key_map[GDK_Page_Up]     = KEY_HANDLER_PAGE_UP;
  key_map[GDK_Page_Down]   = KEY_HANDLER_PAGE_DOWN;
  key_map[GDK_Left]        = KEY_HANDLER_LEFT;
  key_map[GDK_Right]       = KEY_HANDLER_RIGHT;
  key_map[GDK_Up]          = KEY_HANDLER_UP;
  key_map[GDK_Down]        = KEY_HANDLER_DOWN;
  key_map[GDK_KP_Add]      = '+';
  key_map[GDK_KP_Subtract] = '-';
}

void
GtkInterface::unfullscreen()
{
  if (gtk_window != NULL && gtk_window_visible)
    gtk_window_unfullscreen (GTK_WINDOW (gtk_window));
}

void
GtkInterface::toggle_fullscreen()
{
  if (gtk_window != NULL && gtk_window_visible)
    {
      GdkWindowState state = gdk_window_get_state (GDK_WINDOW (gtk_window->window));
      gboolean isFullscreen = ((state & GDK_WINDOW_STATE_FULLSCREEN) == GDK_WINDOW_STATE_FULLSCREEN);
      if (isFullscreen)
        gtk_window_unfullscreen (GTK_WINDOW (gtk_window));
      else
        gtk_window_fullscreen (GTK_WINDOW (gtk_window));
    }
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

void
GtkInterface::show()
{
  if (gtk_window != NULL && !gtk_window_visible)
    {
      gtk_widget_show_all (gtk_window);

      // get cursor, so we can restore it after hiding it
      if (!visible_cursor)
        visible_cursor = gdk_window_get_cursor (GDK_WINDOW (gtk_window->window));

      // sync, to make the window really visible before we return
      gdk_display_sync (gdk_display_get_default());

      gtk_window_visible = true;
    }
}

void
GtkInterface::hide()
{
  if (gtk_window != NULL && gtk_window_visible)
    {
      gtk_widget_hide_all (gtk_window);
      gtk_window_visible = false;
    }
}

void
GtkInterface::resize (int x, int y)
{
  if (gtk_window != NULL)
    gtk_window_resize (GTK_WINDOW (gtk_window), x, y);
}

bool
GtkInterface::handle_keypress_event (GdkEventKey *event)
{
  int ch = 0;

  if (event->keyval > 0 && event->keyval <= 127)
    ch = event->keyval;
  else
    ch = key_map[event->keyval];

  if (ch != 0)
    {
      key_handler->process_input (ch);
      return true;
    }
  return false;
}

void
GtkInterface::set_title (const string& title)
{
  if (gtk_window != NULL)
    gtk_window_set_title (GTK_WINDOW (gtk_window), title.c_str());
}

bool
GtkInterface::handle_timeout()
{
  if (gtk_window != NULL && gtk_window_visible)
    {
      if (cursor_timeout == 0)
        {
          gdk_window_set_cursor (GDK_WINDOW (gtk_window->window), invisible_cursor);
          cursor_timeout = -1;
        }
      else if (cursor_timeout > 0)
        {
          cursor_timeout--;
        }
    }
  return true;
}

bool
GtkInterface::handle_motion_notify_event (GdkEventMotion *event)
{
  if (gtk_window != NULL && gtk_window_visible)
    {
      gdk_window_set_cursor (GDK_WINDOW (gtk_window->window), visible_cursor);
      cursor_timeout = 3;
    }
  return true;
}

bool
GtkInterface::handle_close()
{
  g_return_val_if_fail (gtk_window != NULL, true);

  // quit on close
  key_handler->process_input ('q');

  return true;
}
