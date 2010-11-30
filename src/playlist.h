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

#ifndef __GST123_PLAYLIST_H__
#define __GST123_PLAYLIST_H__

#include "uri.h"
#include <vector>
#include <string>

namespace Gst123 {

enum
{
  PLAYLIST_PARSER_NOTIMPL = -1
};

struct PlaylistParser
{
  virtual int parse (std::vector<std::string>& output, IOStream *stream) = 0;
  virtual bool identify (IOStream *stream) = 0;
  virtual std::string str_error (int error = 0) = 0;

protected:
  int status;
};

class Playlist : public std::vector<std::string>
{
  std::vector<PlaylistParser *> parser_register;
  PlaylistParser *current_parser;

  int parse (URI &uri);
  void register_parsers (void);
  int error;
public:
  Playlist (const std::string& uri_str);
  bool is_valid();

  ~Playlist (void)
  {
    for (unsigned int i = 0; i < parser_register.size(); i++)
      delete parser_register[i];
  }
};

}

// The list of parsers
#include "plsparser.h"
#include "m3uparser.h"

#endif
