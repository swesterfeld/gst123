/* GST123 - GStreamer based command line media player
 * SPDX-FileCopyrightText: 2010 Stefan Westerfeld
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
#include "configfile.h"
#include "microconf.h"

#include <stdlib.h>

using std::string;

static ConfigFile *instance = 0;

ConfigFile&
ConfigFile::the()
{
  if (!instance)
    instance = new ConfigFile();

  return *instance;
}

string
ConfigFile::audio_output() const
{
  return m_audio_output;
}

string
ConfigFile::visualization() const
{
  return m_visualization;
}

ConfigFile::ConfigFile()
{
  char *home = getenv ("XDG_CONFIG_HOME");
  string filename;

  if (home)
    {
      filename = home;
      filename += "/gst123rc";
    }
  else
    {
      home = getenv ("HOME");
      if (!home)
        return;
      filename = home;
      filename += "/.config/gst123rc";
    }

  MicroConf cfg (filename);
  if (!cfg.open_ok())
    {
      // thats OK, we don't need a config file if the user doesn't have one
      return;
    }
  while (cfg.next())
    {
      string str;
      if (cfg.command ("audio_output", str))
        {
          m_audio_output = str;
        }
      else if (cfg.command ("visualization", str))
        {
          m_visualization = str;
        }
      else
        {
          cfg.die_if_unknown();
        }
    }
}
