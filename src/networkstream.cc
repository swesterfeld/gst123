/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006-2010 Stefan Westerfeld
 * Copyright (C) 2010 أحمد المحمودي (Ahmed El-Mahmoudy)
 *
 * Playlist support: GstNetworkStream
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
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <glib.h>
#include <errno.h>

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

using namespace Gst123;

NetworkStream::NetworkStream (const string& host, int port)
{
  this->host = host;
  this->port = port;
  openStream ();
}

NetworkStream::~NetworkStream()
{
  close (fd);
}

void
NetworkStream::openStream()
{
  struct addrinfo hints;
  struct addrinfo *result = NULL, *rp = NULL;
  char port_str[10];
  int save_errno = 0, ret = 0;

  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  snprintf(port_str, sizeof(port), "%d", port);

  if ((ret = getaddrinfo(host.c_str (), port_str, &hints, &result)) < 0)
    {
      cerr << "Connect: unable to create socket for "
           << host << ":" << port << "(" << gai_strerror(ret) << ")"
           << endl;
    }
  else
    {
      for (rp = result; rp; rp = rp->ai_next)
        {
          fd = socket(rp->ai_family, rp->ai_socktype,
                      rp->ai_protocol);

          if (fd == -1)
            continue;

          if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
            {
              freeaddrinfo(result);
              return;
            }
          else
            save_errno = errno;
        }
    }

  if (result)
    freeaddrinfo(result);

  if (save_errno)
    cerr << "Connect: unable to connect to "
         << host << ":" << port << "(" << gai_strerror(ret) << ")"
         << endl;
}

