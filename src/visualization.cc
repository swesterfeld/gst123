/* GST123 - GStreamer based command line media player
 * SPDX-FileCopyrightText: 2012 Stefan Westerfeld
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

#include <gst/gst.h>
#include <stdio.h>

#include "compat.h"
#include "options.h"
#include "visualization.h"

using namespace Gst123;

namespace Visualization
{

static gboolean
filter_features (GstPluginFeature * feature, gpointer data)
{
  GstElementFactory *f;

  if (!GST_IS_ELEMENT_FACTORY (feature))
    return FALSE;
  f = GST_ELEMENT_FACTORY (feature);
  if (!g_strrstr (gst_element_factory_get_klass (f), "Visualization"))
    return FALSE;

  return TRUE;
}

static GList *
get_visualization_features()
{
  return gst_registry_feature_filter (Compat::registry_get(), filter_features, FALSE, NULL);
}

void
print_visualization_list()
{
  GList *l = get_visualization_features();
  while (l)
    {
      printf ("%-30s %s\n",
        (gchar *) gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (l->data)),
        (gchar *) gst_element_factory_get_longname (GST_ELEMENT_FACTORY (l->data)));
      l = g_list_next (l);
    }
  g_list_free (l);
}

bool
setup (GstElement *playbin)
{
  GstElement *vis_plugin = gst_element_factory_make (Options::the().visualization, "visplugin");
  if (!vis_plugin)
    return false;

  int flags;
  g_object_get (G_OBJECT (playbin), "flags", &flags, NULL);
  const int GST_PLAY_FLAGS_VIS = 0x08; // FIXME: is there a better way than hardcoding this?
  flags |= GST_PLAY_FLAGS_VIS;
  g_object_set (G_OBJECT (playbin), "flags", flags, NULL);
  g_object_set (G_OBJECT (playbin), "vis-plugin", vis_plugin, NULL);

  return true;
}

}
