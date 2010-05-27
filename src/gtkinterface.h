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
#ifndef GST123_GTK_INTERFACE_H
#define GST123_GTK_INTERFACE_H

#include <gtk/gtk.h>
#include "keyhandler.h"
#include <map>
#include <string>

class GtkInterface
{
  GtkWidget   *gtk_window;
  bool         gtk_window_visible;
  KeyHandler  *key_handler;
  GdkCursor   *invisible_cursor;
  GdkCursor   *visible_cursor;
  int          cursor_timeout;      // number of timeout events until we hide the cursor (-1 if already hidden)

  std::map<int,int>   key_map;

public:
  void init (int *argc, char ***argv, class KeyHandler *key_handler);
  void show();
  void hide();
  bool init_ok();
  GtkWidget *window();
  void unfullscreen();
  void toggle_fullscreen();
  bool handle_keypress_event (GdkEventKey *event);
  bool handle_motion_notify_event (GdkEventMotion *event);
  bool handle_timeout();
  bool handle_close();
  void resize (int x, int y);
  void set_title (const std::string& title);
};

#endif
