/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006-2010 Stefan Westerfeld
 * Copyright (C) 2010 أحمد المحمودي (Ahmed El-Mahmoudy)
 *
 * Playlist support: PLS playlist parser
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

#include "plsparser.h"
#include <ctype.h>
#include <cstring>

const string PLSParser::type = "audio/x-scpls";

bool
PLSParser::identify (GstIOStream *stream)
{
  // Trust the content type first
  if (stream->getContentType () == PLSParser::type)
    return true;
  else if (stream->getContentType () != "")
    return false;

  if (stream->contentBeginsWith("[playlist]"))
    return true;
  else
    return false;
}

// Parse the playlist
// Currently we are only picking out the file to be played
// and we don't really care about the playlist entry information
// such as the title, artist, etc.
int
PLSParser::parse (vector<string> &list, GstIOStream *stream)
{
  do
    {
      string curline = stream->getCurrentLine ();

      if (curline == "")
	continue;

      while (curline[0] && isspace (curline[0]))
        curline.erase(0, 1);

      if (curline.substr(0, 4) == "File")
	{
	  int pos = curline.find("=");
	  string ret = curline.substr(pos+1, string::npos);
	  while (ret[0] && isspace (ret[0]))
	    ret.erase (0, 1);

	  list.push_back(ret);
	}
    }
  while (stream->readline () >= 0);

  return 0;
}
