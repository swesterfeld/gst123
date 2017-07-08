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
#ifndef GST123_KEY_HANDLER_H
#define GST123_KEY_HANDLER_H

/* key codes for process input ; everything < 256 is plain ascii */
enum {
  KEY_HANDLER_BACKSPACE = 0177, // ASCII backspace
  KEY_HANDLER_UP = 300,
  KEY_HANDLER_LEFT,
  KEY_HANDLER_RIGHT,
  KEY_HANDLER_DOWN,
  KEY_HANDLER_PAGE_UP,
  KEY_HANDLER_PAGE_DOWN
};

class KeyHandler
{
public:
  virtual void process_input (int ch) = 0;
};

#endif
