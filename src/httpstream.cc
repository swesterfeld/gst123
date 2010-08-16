/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006-2010 Stefan Westerfeld
 * Copyright (C) 2010 أحمد المحمودي (Ahmed El-Mahmoudy)
 *
 * Playlist support: GstHTTPStream
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

#include "gstiostream.h"
#include <glib.h>
#include <cstring>
#include <errno.h>

#include <iostream>
using std::cerr;
using std::endl;

GstHTTPStream :: GstHTTPStream (const string &host, int port, const string &path)
               : GstNetworkStream (host, port)
{
  this->path = path;
  setupHttp ();
  httpReadHeaders ();
}

string
GstHTTPStream :: getContentType (void)
{
  return headers ["Content-Type"];
}

string
GstHTTPStream :: getHeaderValue (const string & name)
{
  return headers[name];
}

int
GstHTTPStream :: getResponseCode (void)
{
  return responsecode;
}

// Send the HTTP request
void
GstHTTPStream :: setupHttp (void)
{
  char *buf = g_strdup_printf ("GET %s HTTP/1.0\r\n"
                               "Host: %s\r\n"
                               "User-Agent: gst123\r\n\r\n",
                               path.c_str (), host.c_str ());

  int ret = write (fd, buf, strlen (buf));
  g_free (buf);

  if (ret == -1)
    {
      cerr << "HTTP Request failed: " << strerror(errno) << endl;
      return;
    }

}

// Read and parse HTTP headers
void
GstHTTPStream :: httpReadHeaders (void)
{
  string line;
  char mode[8];
  char message[256];

  readline ("\r\n");

  line = getCurrentLine ();

  // Our first line: HTTP <responsecode> <description>
  if (line != "")
    sscanf(line.c_str(), "%s %d %s", mode, &responsecode, message);
  else
    responsecode = -1;

  // HTTP Headers
  while ((readline ("\r\n") > 0))
    {
      string name, value;

      line = getCurrentLine ();
      int sep = line.find(": ");

      name = line.substr (0, sep);
      value = line.substr (sep + 2);

      headers [name] = value;
    }
}

/*
 * Response strings for some basic HTTP response codes
 * The list is not complete, but sufficient for a majority
 * of cases
 */
string
GstHTTPStream::getResponse(int error)
{
  switch (error)
    {
    case 404:
      return "HTTP: Not Found";
    case 500:
      return "HTTP: Internal Server Error";
    case 401:
    case 403:
      return "HTTP: Forbidden";
    case 301:
    case 302:
      return "HTTP: Content moved to another location";
    default:
      return "HTTP: Error code " + error;;
    }
}

