/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006-2010 Stefan Westerfeld
 * Copyright (C) 2010 أحمد المحمودي (Ahmed El-Mahmoudy)
 *
 * Playlist support: GstIOStream
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

#include <cstdio>
#include "iostream.h"
#include <errno.h>
#include <glib.h>
#include <cstring>

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

using namespace Gst123;

IOStream::IOStream()
{
  bof = true;
  eof = false;
  fd = -1;
}

IOStream::~IOStream()
{
}

string
IOStream::get_content_type()
{
  return "";
}

/*
 * Read a line of input from in
 * The newline string is to specify the separator we have
 * between lines. It is \n by default, but can be set to
 * "\r\n" or any other arbitrary string
 *
 * Returns length of the line read
 */
int
IOStream::readline (const string& newline)
{
  char buf [4096];
  int len = -1;
  size_t pos = string::npos;

  if (!eof)
    {
      do
        {
          len = read (fd, buf, sizeof (buf) - 1);

          if (len == 0)
            break;

          if (len < 0)
            {
              cerr << "Read error on fd " << fd
                   << ": " << strerror (errno) << endl;
              return IO_STREAM_ERROR;
            }

          buf[len] = '\0';

          strbuf += buf;
        }
      while ((pos = strbuf.find (newline)) == string::npos);
    }
  else if (strbuf == "")
    return IO_STREAM_EOF;

  pos = strbuf.find (newline);

  if (pos == string::npos)
    {
      curline = strbuf;
      strbuf = "";
      eof = true;
    }
  else
    {
      curline = strbuf.substr (0, pos);
      strbuf = strbuf.substr (pos + newline.length(), string::npos);
    }

  bof = false;
  return curline.length();
}

/* Look for a specific pattern in the first line of content */
bool
IOStream::content_begins_with (const string& match)
{
  if (bof)
    readline();

  return (curline.find (match) == 0);
}

std::string&
IOStream::get_current_line()
{
  return curline;
}
