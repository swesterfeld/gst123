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

#include "iostream.h"
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <glib.h>

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

using namespace Gst123;

FileStream::FileStream (const std::string& path)
{
  this->path = path;

  open_stream();
}

FileStream::~FileStream()
{
  close (fd);
}

void
FileStream::open_stream()
{
  fd = open (path.c_str(), O_RDONLY);
  if (fd < 0)
    status = errno;
}

string
FileStream::str_error_impl(int error)
{
  return "Unable to open file " + path + ": " + strerror(error);
}
