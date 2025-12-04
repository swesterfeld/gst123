/* GST123 - GStreamer based command line media player
 * SPDX-FileCopyrightText: 2016 Stefan Westerfeld
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

#include <sys/time.h>
#include "utils.h"

using std::string;

namespace Gst123
{

double
get_time()
{
  timeval tv;
  gettimeofday (&tv, 0);

  return double (tv.tv_sec) + double (tv.tv_usec) * (1.0 / 1000000.0);
}

string
string_printf (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  char *c_str = g_strdup_vprintf (format, args);
  va_end (args);

  string str = c_str;
  g_free (c_str);

  return str;
}

}
