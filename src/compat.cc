/* GST123 - GStreamer based command line media player
 * Copyright (C) 2013 Stefan Westerfeld
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

#include "compat.h"

#include <gst/video/video.h>

#if GST_CHECK_VERSION(1,0,0)
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif

using namespace Gst123;

#if GST_CHECK_VERSION(1,0,0)

bool
Compat::element_query_position (GstElement *element, GstFormat format, gint64 *cur_pos)
{
  return gst_element_query_position (element, format, cur_pos);
}

bool
Compat::element_query_duration (GstElement *element, GstFormat format, gint64 *cur_pos)
{
  return gst_element_query_duration (element, format, cur_pos);
}

bool
Compat::video_get_size (GstPad *pad, int *width, int *height)
{
  bool result = false;

  if (GstCaps *caps = gst_pad_get_current_caps (pad))
    {
      GstVideoInfo info;

      gst_video_info_init (&info);
      // get video size (if any)
      if (gst_video_info_from_caps (&info, caps))
        {
          *width = info.width;
          *height = info.height;

          result = true;
        }

      gst_caps_unref (caps);
    }
  return result;
}

void
Compat::iterator_foreach (GstIterator *iterator, void (*func) (GstElement *element, gpointer user_data), gpointer user_data)
{
}

void
Compat::video_overlay_set_window_handle (GstMessage *msg, guintptr id)
{
}

GstElement*
Compat::create_playbin (const char *name)
{
  return gst_element_factory_make ("playbin2", name);
}

bool
Compat::is_stream_start_message (GstMessage *msg)
{
  return false;
}

GstCaps*
Compat::pad_get_current_caps (GstPad *pad)
{
  return gst_pad_get_current_caps (pad);
}

bool
Compat::is_video_overlay_prepare_window_handle_message (GstMessage *msg)
{
  return false;
}

GstRegistry*
Compat::registry_get()
{
  return gst_registry_get();
}


#else

bool
Compat::element_query_position (GstElement *element, GstFormat format, gint64 *cur_pos)
{
  return gst_element_query_position (element, &format, cur_pos);
}

bool
Compat::element_query_duration (GstElement *element, GstFormat format, gint64 *cur_pos)
{
  return gst_element_query_duration (element, &format, cur_pos);
}

bool
Compat::video_get_size (GstPad *pad, int *width, int *height)
{
  return gst_video_get_size (GST_PAD (pad), width, height);
}

void
Compat::iterator_foreach (GstIterator *iterator, void (*func) (GstElement *element, gpointer user_data), gpointer user_data)
{
}

void
Compat::video_overlay_set_window_handle (GstMessage *msg, guintptr id)
{
  gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (GST_MESSAGE_SRC (msg)), id);
}

GstElement*
Compat::create_playbin (const char *name)
{
  return gst_element_factory_make ("playbin2", name);
}

bool
Compat::is_stream_start_message (GstMessage *msg)
{
  return GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ELEMENT &&
         gst_structure_has_name (msg->structure, "playbin2-stream-changed");
}

GstCaps*
Compat::pad_get_current_caps (GstPad *pad)
{
  return gst_pad_get_negotiated_caps (pad);
}

bool
Compat::is_video_overlay_prepare_window_handle_message (GstMessage *msg)
{
  return GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ELEMENT &&
         gst_structure_has_name (msg->structure, "prepare-xwindow-id");
}

GstRegistry*
Compat::registry_get()
{
  return gst_registry_get_default();
}


#endif
