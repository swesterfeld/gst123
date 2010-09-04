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

#include "m3uparser.h"
#include <ctype.h>
#include <cstring>

using std::string;
using std::vector;

using namespace Gst123;

const string M3UParser::type = "audio/x-mpegurl";

// Identify if the playlist is M3U
bool
M3UParser::identify (IOStream *stream)
{
  // Trust the content type first
  if (stream->get_content_type() == M3UParser::type)
    return true;
  else if (stream->get_content_type() != "")
    return false;

  if (stream->content_begins_with ("#EXTM3U"))
    extended = true;
  else
    extended = false;

  return true;
}

// Parse the playlist
// Currently we are only picking out the file to be played
// and we don't really care about the playlist entry information
// such as the title, artist, etc.
int
M3UParser::parse (vector<string>& list, IOStream *stream)
{
  do
    {
      string curline = stream->get_current_line();

      if (curline == "")
	continue;

      while (curline[0] && isspace (curline[0]))
        curline.erase (0, 1);

      // Avoid comments
      if (curline[0] && curline[0] != '#')
        list.push_back (curline);
    }
  while (stream->readline() >= 0);

  return 0;
}
