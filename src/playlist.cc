/* GST123 - GStreamer based command line media player
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

using namespace Gst123;

Playlist::Playlist (const string& uri_str)
{
  string errorstr;

  register_parsers();
  URI uri (uri_str);

  error = uri.open();

  if (error)
    {
      cerr << "Playlist Error: " << uri.strerror(error) << endl;
      return;
    }

  error = parse (uri);

  if (error == PLAYLIST_PARSER_NOTIMPL)
    cerr << "Playlist Error: Parser not implemented for this playlist type" << endl;
  else if (error)
    {
      string err = current_parser->str_error (error);

      if (err == "")
        err = uri.read_strerror (error);

      cerr << "Playlist Error: " << err << endl;
    }
}

void
Playlist::register_parsers()
{
  current_parser = NULL;
  parser_register.push_back(new PLSParser());

  // Make sure that this is last. It acts as a catch-all since the format
  // is simply one entry per line.
  parser_register.push_back(new M3UParser());
}

bool
Playlist::is_valid()
{
  if (error)
    return false;
  else
    return true;
}

int
Playlist::parse (URI &uri)
{
  IOStream *stream = uri.get_io_stream ();

  for (size_t i = 0; i < parser_register.size(); i++)
    {
      if (parser_register[i]->identify (stream))
        {
          current_parser = parser_register[i];
          return parser_register[i]->parse (*this, stream);
        }
    }

  return PLAYLIST_PARSER_NOTIMPL;
}
