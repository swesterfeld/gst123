/* GST123 - GStreamer based command line media player
 * Copyright (C) 2012 Stefan Westerfeld
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

#ifndef __GST123_TYPE_FINDER__
#define __GST123_TYPE_FINDER__

#include <gst/gst.h>
#include <string>

namespace Gst123
{

class TypeFinder
{
private:
  void typefound (const std::string& type, guint probability);
  void run (const std::string& filename);

  static void cb_typefound (GstElement *typefind,
                            guint       probability,
                            GstCaps    *caps,
                            gpointer    data);

  std::string   m_type;
  std::string   m_subtype;
  guint    m_probability;
  GMutex  *mutex;
  GCond   *cond;
  bool     done;

public:
  TypeFinder (const std::string& filename);
  ~TypeFinder();

  std::string
  type() const
  {
    return m_type;
  }
  std::string
  subtype() const
  {
    return m_subtype;
  }
  guint
  probability() const
  {
    return m_probability;
  }
};

}

#endif
