/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006-2010 Stefan Westerfeld
 * Copyright (C) 2010 أحمد المحمودي (Ahmed El-Mahmoudy)
 *
 * Playlist support: Playlist base object
 * Copyright (C) 2010 Siddhesh Poyarekar
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

#include "playlist.h"
#include <iostream>

using std::cerr;
using std::endl;
using std::string;


Playlist :: Playlist (const string& uri_str)
{
  string errorstr;

  register_parsers ();
  URI uri(uri_str);
  int error = uri.open();

  if (!error)
    error = parse (uri);
  else
    {
      errorstr = uri.strerror(error);
      error = 0;
    }

  if (error)
    errorstr = "Parser not implemented for this playlist type";

  if (errorstr != "")
    cerr << "Playlist Error: " << errorstr << endl;
}

void
Playlist::register_parsers (void)
{
  parser_register.push_back(new PLSParser ());

  // Make sure that this is last. It acts as a catch-all since the format
  // is simply one entry per line.
  parser_register.push_back(new M3UParser ());
}

int
Playlist::parse (URI &uri)
{
  GstIOStream *stream = uri.getIOStream ();

  for (size_t i = 0; i < parser_register.size(); i++)
    {
      if (parser_register[i]->identify (stream))
	{
	  parser_register[i]->parse (*this, stream);
	  return 0;
	}
    }

  return PLAYLIST_PARSER_NOTIMPL;
}

