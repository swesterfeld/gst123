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

using namespace Gst123;

ConsoleStream::ConsoleStream (FILE *f)
{
  fd = fileno (f);
}

void
ConsoleStream::open_stream()
{
// Do nothing. It's already open.
}

std::string
ConsoleStream::str_error_impl(int error)
{
  return "";
}
