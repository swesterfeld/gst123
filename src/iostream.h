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

#ifndef __GST123_IO_STREAM__
#define __GST123_IO_STREAM__

#include <string>
#include <cstdio>
#include <map>

namespace Gst123
{

enum
{
  IO_STREAM_EOF = -1,
  IO_STREAM_ERROR = -2
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
class IOStream
{
public:
  IOStream();

  int readline (const std::string& separator = "\n");
  bool contentBeginsWith (const std::string& magic);
  virtual std::string getContentType();
  std::string& getCurrentLine();

protected:
  int fd;
  virtual void openStream() = 0;
private:
  std::string curline;
  std::string strbuf;
  bool bof;
  bool eof;
};

// File I/O
class FileStream : public IOStream
{
public:
  FileStream (const std::string& path);
  ~FileStream();

protected:
  void openStream();

private:
  std::string path;
};

// Raw Network I/O
class NetworkStream : public IOStream
{
public:
  NetworkStream (const std::string& host, int port);
  ~NetworkStream();

protected:
  std::string host;
  int port;

  void openStream();
};

// HTTP I/O stream
class HTTPStream : public NetworkStream
{
public:
  HTTPStream (const std::string& host, int port, const std::string& path);

  std::string getHeaderValue(const std::string& name);
  int getResponseCode();
  std::string getContentType();

  static std::string getResponse (int error);

private:
  std::string path;
  int responsecode;
  std::map<std::string, std::string> headers;

  void setupHttp();
  void httpReadHeaders();
};

// Console I/O
class ConsoleStream : public IOStream
{
public:
  ConsoleStream (FILE *f);

protected:
  void openStream();
};

}

#endif /* __GST123_IO_STREAM__ */
