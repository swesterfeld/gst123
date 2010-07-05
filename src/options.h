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
#ifndef GST123_OPTIONS_H
#define GST123_OPTIONS_H

#include <string>
#include <list>
#include <glib.h>

class Options
{
  static Options *instance;

  static void print_version();
  static void add_playlist (const gchar *option_name, const gchar *value);

public:
  std::string	program_name; /* FIXME: what to do with that */
  std::string   usage;

  // variables filled via command line options:
  gboolean      verbose;
  gboolean      shuffle;
  gboolean      novideo;
  char        **uris;
  std::list<std::string>  playlists;
  char         *audio_output;

  Options ();
  void parse (int argc, char **argv);
};

#endif
