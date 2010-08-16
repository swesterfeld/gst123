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

#ifndef __GST_IO_STREAM__
#define __GST_IO_STREAM__

#include <string>
#include <cstdio>
#include <map>

using std::string;
using std::map;

enum
{
  GST_IO_STREAM_EOF = -1,
  GST_IO_STREAM_ERROR = -2
};

/*
 * I/O Stream classes
 *
 * These are primarily input stream classes that
 * provide a unified way to read from various types of
 * sources. This can be extended to do full I/O once
 * the need for it arises. For now, they're good the
 * way they are.
 *
 * To add a new network I/O source (ftp for example),
 * simply derive a class from GstNetworkStream and define
 * an openStream () that does this:
 *
 * 1) calls the openStream () of the parent class to establish a
 *    connection
 * 2) Do whatever request/responses required for the
 *    higher level protocol and make the stream ready for
 *    input
 */

// Base class for I/O Streams
class GstIOStream
{
public:
  GstIOStream (void);
  int readline (const string & separator = "\n");
  bool contentBeginsWith (const string magic);
  virtual string getContentType (void);
  string &getCurrentLine (void);

protected:
  int fd;
  virtual void openStream (void) = 0;
private:
    string curline;
    string strbuf;
    bool bof;
    bool eof;
};

// File I/O
class GstFileStream:public GstIOStream
{
public:
  GstFileStream (const string & path);
  ~GstFileStream (void);

protected:
  void openStream (void);

private:
  string path;
};

// Raw Network I/O
class GstNetworkStream:public GstIOStream
{
public:
  GstNetworkStream (const string & host, int port);

  ~GstNetworkStream (void);

protected:
    string host;
  int port;

  void openStream (void);
};

// HTTP I/O stream
class GstHTTPStream:public GstNetworkStream
{
public:
  GstHTTPStream (const string & host, int port, const string & path);

  string getHeaderValue (const string & name);
  int getResponseCode (void);
  string getContentType (void);

  static string getResponse(int error);

private:
  string path;
  int responsecode;
  map < string, string > headers;

  void setupHttp (void);
  void httpReadHeaders (void);
};

// Console I/O
class GstConsoleStream:public GstIOStream
{
public:
  GstConsoleStream (FILE * f);

protected:
  void openStream (void);
};

#endif /* __GST_IO_STREAM__ */
