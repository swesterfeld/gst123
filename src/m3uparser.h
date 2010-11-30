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

#ifndef __M3U_PARSER_H__
#define __M3U_PARSER_H__

#include "playlist.h"

namespace Gst123
{

struct M3UParser : public PlaylistParser
{
  int parse (std::vector<std::string>& list, IOStream *stream);
  bool identify (IOStream *stream);
  std::string str_error (int error);
private:
  bool extended;
  static const std::string type;
};

}

#endif
