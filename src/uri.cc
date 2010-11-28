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

#include <string>
#include <cstdio>

#include "uri.h"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

using namespace Gst123;

/*
 * Constructor:
 * Parse the input URI and save it as a combination of
 * host, port, protocol and path
 * Following formats work:
 *
 *         http://server:port/path/to/file
 *         http://server/path/to/file
 *         http://server:port
 *         http://server
 *         file:///path/to/file
 *         -
 * where - signifies stdin
 */
URI::URI (const string &input)
{
  size_t start = string::npos;
  size_t walk = string::npos;
  size_t portsep = string::npos;

  stream = NULL;
  status = 0;

  if (input == "-")
    {
      protocol = "stdin";
      return;
    }

  if ((start = input.find("://")) == string::npos)
    {
      path = input;
      return;
    }

  protocol = input.substr(0, start);
  start += 3;

  if ((walk = input.find("/", start)) == string::npos && !empty_path_allowed ())
    {
      status = URI_ERROR_INVALID_URI;
      return;
    }
  else if (walk == string::npos)
    path = "/";
  else
    path = input.substr(walk);

  portsep = input.find (":", start, walk);

  if (portsep != string::npos)
    {
      host = input.substr (start, portsep - start);
      string port_str = input.substr (portsep, walk - portsep);
      port = atoi (port_str.c_str());
    }
  else
    {
      host = input.substr (start, walk - start);
      if (protocol == "http")
        port = 80;
    }

  if (protocol == "http" && host == "")
    {
      status = URI_ERROR_INVALID_HTTP;
      return;
    }
}

URI::~URI()
{
  if (stream)
    delete stream;
}

/*
 * Open URI:
 * Based on the protocol, it either opens an HTTP stream or a
 * local file stream
 *
 * Return value:
 *
 * =0: Success
 * <0: Network/lower level error
 * >0: HTTP error
 */
int
URI::open()
{
  // Don't even bother if we have already encountered an error
  if (status)
    return status;

  if (protocol == "http")
    {
      if (host == "")
        return (status = URI_ERROR_INVALID_HOST);

      stream = new HTTPStream (host, port, path);
    }

  else if (protocol == "stdin")
    stream = new ConsoleStream (stdin);
  else
    {
      if (path == "")
        return (status = URI_ERROR_INVALID_PATH);

      stream = new FileStream (path);
    }

  return stream->get_status ();
}

bool
URI::empty_path_allowed()
{
  if (protocol == "http")
    return true;

  return false;
}

IOStream *
URI::get_io_stream()
{
  return stream;
}

string
URI::strerror (int error)
{
  if (!status && stream)
    {
      return stream->str_error(error);
    }

  switch (error)
    {
    case URI_ERROR_INVALID_URI:
      return "URI: Invalid URI format";
    case URI_ERROR_INVALID_HOST:
      return "URI: Invalid Host name";
    case URI_ERROR_INVALID_HTTP:
      return "URI: Invalid HTTP response";
    case URI_ERROR_INVALID_PATH:
      return "URI: Invalid URI path";
    default:
      return "URI: Unknown Error";
    }
}

string
URI::read_strerror (int error)
{
  return stream->read_str_error (error);
}
