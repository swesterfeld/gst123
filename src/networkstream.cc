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
#include "utils.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <glib.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

using namespace Gst123;

NetworkStream::NetworkStream (const string& host, int port)
{
  this->host = host;
  this->port = port;
  this->lookup_error = false;
  open_stream();
}

NetworkStream::~NetworkStream()
{
  close (fd);
}

void
NetworkStream::open_stream()
{
  struct addrinfo hints;
  struct addrinfo *result = NULL, *rp = NULL;
  int ret = 0;

  memset (&hints, 0, sizeof (struct addrinfo));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  string port_str = string_printf ("%d", port);

  if ((ret = getaddrinfo (host.c_str(), port_str.c_str(), &hints, &result)) < 0)
    {
      status = ret;
      lookup_error = true;
    }
  else
    {
      for (rp = result; rp; rp = rp->ai_next)
        {
          fd = socket (rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);

          if (fd == -1)
	    {
	      status = errno;
              continue;
	    }

          if (connect (fd, rp->ai_addr, rp->ai_addrlen) != -1)
            {
              freeaddrinfo (result);
	      status = 0;
              return;
            }
          else
            status = errno;
        }
    }

  if (result)
    freeaddrinfo (result);
}

string
NetworkStream::str_error (int error)
{
  return net_error (error);
}

string
NetworkStream::net_error (int error)
{
  string str = "Network Error: ";

  if (!error)
    error = status;

  if (lookup_error)
    {
      str += string_printf ("Failed to look up host %s:%d (%s)", host.c_str(), port, gai_strerror (error));
      return str;
    }

  return str + strerror (error);
}
