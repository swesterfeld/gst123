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

#ifndef __GST123_COMPAT__
#define __GST123_COMPAT__

#include <gst/gst.h>

namespace Gst123
{

namespace Compat
{

typedef void (*IteratorFunc) (GstElement *element, gpointer user_data);

bool element_query_position (GstElement *element, GstFormat format, gint64 *cur_pos);
bool element_query_duration (GstElement *element, GstFormat format, gint64 *cur_pos);
bool video_get_size (GstPad *pad, int *width, int *height);
bool is_video_overlay_prepare_window_handle_message (GstMessage *msg);
bool is_stream_start_message (GstMessage *msg);

void iterator_foreach (GstIterator *iterator, IteratorFunc func, gpointer user_data);
void video_overlay_set_window_handle (GstMessage *msg, guintptr id);

GstElement* create_playbin (const char *name);
GstCaps *pad_get_current_caps (GstPad *pad);
GstRegistry *registry_get();

}

}

#endif
