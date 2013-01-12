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

#include "typefinder.h"
#include <unistd.h>
#include <string>

using std::string;

namespace Gst123
{

TypeFinder::TypeFinder (const string& filename)
{
  done = false;

  m_probability = 0;
  g_mutex_init (&mutex);
  g_cond_init (&cond);

  run (filename);
}

TypeFinder::~TypeFinder()
{
  g_cond_clear (&cond);
  g_mutex_clear (&mutex);
}

void
TypeFinder::typefound (const string& type, guint probability)
{
  size_t split_pos = type.find ('/');
  if (split_pos != string::npos)
    {
      m_type = type.substr (0, split_pos);
      m_subtype = type.substr (split_pos + 1);
      m_probability = probability;
    }

  g_mutex_lock (&mutex);
  done = true;
  g_cond_signal (&cond);
  g_mutex_unlock (&mutex);
}

void
TypeFinder::cb_typefound (GstElement *typefind,
                          guint       probability,
                          GstCaps    *caps,
                          gpointer    data)
{
  TypeFinder *self = (TypeFinder *) data;

  gchar *type = gst_caps_to_string (caps);
  self->typefound (type, probability);
  g_free (type);
}

void
TypeFinder::run (const string& filename)
{
  /* create a new pipeline to hold the elements */
  GstElement *pipeline = gst_pipeline_new ("pipe");

  /* create file source and typefind element */
  GstElement *filesrc = gst_element_factory_make ("filesrc", "source");
  g_object_set (G_OBJECT (filesrc), "location", filename.c_str(), NULL);

  GstElement *typefind = gst_element_factory_make ("typefind", "typefinder");
  g_signal_connect (typefind, "have-type", G_CALLBACK (cb_typefound), this);

  GstElement *fakesink = gst_element_factory_make ("fakesink", "sink");

  /* setup */
  gst_bin_add_many (GST_BIN (pipeline), filesrc, typefind, fakesink, NULL);
  gst_element_link_many (filesrc, typefind, fakesink, NULL);
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

  g_mutex_lock (&mutex);
  gint64 end_time = g_get_monotonic_time() + 500 * G_TIME_SPAN_MILLISECOND;
  while (!done)
    {
      if (!g_cond_wait_until (&cond, &mutex, end_time))
        {
          // timeout occurred
          break;
        }
    }
  g_mutex_unlock (&mutex);

  /* unset */
  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
}

}
