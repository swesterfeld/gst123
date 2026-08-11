// SPDX-License-Identifier: LGPL-2.0-or-later
/* GST123 - GStreamer based command line media player
 * Copyright (C) 2026 Stefan Westerfeld
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

#include "idleinhibitor.h"

using namespace Gst123;

IdleInhibitor::~IdleInhibitor()
{
  uninhibit();

  if (proxy_)
    g_object_unref (proxy_);
}

bool
IdleInhibitor::inhibit()
{
  if (cookie_ != 0)
    return true; // Already inhibited.

  GError* error = nullptr;

  if (!proxy_)
    {
      proxy_ = g_dbus_proxy_new_for_bus_sync (
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.freedesktop.ScreenSaver",
        "/org/freedesktop/ScreenSaver",
        "org.freedesktop.ScreenSaver",
        nullptr,
        &error
      );

      if (!proxy_)
        {
          // fail silently
          // g_warning ("Could not create ScreenSaver D-Bus proxy: %s", error ? error->message : "unknown error");
          g_clear_error (&error);
          return false;
        }
    }

  GVariant* result = g_dbus_proxy_call_sync (
    proxy_,
    "Inhibit",
    g_variant_new ("(ss)", "gst123", "Playing a video"),
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    nullptr,
    &error
  );

  if (!result)
    {
      // fail silently
      // g_warning ("Could not inhibit idle: %s", error ? error->message : "unknown error");
      g_clear_error (&error);
      return false;
    }

  guint32 cookie = 0;
  g_variant_get (result, "(u)", &cookie);
  g_variant_unref (result);

  cookie_ = cookie;

  return true;
}

void
IdleInhibitor::uninhibit()
{
  if (!proxy_ || cookie_ == 0)
    return;

  GError* error = nullptr;

  GVariant* result = g_dbus_proxy_call_sync (
    proxy_,
    "UnInhibit",
    g_variant_new("(u)", cookie_),
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    nullptr,
    &error
  );

  if (!result)
    {
      // fail silently
      // g_warning ("Could not remove idle inhibition: %s", error ? error->message : "unknown error");
      g_clear_error (&error);
    }
  else
    {
      g_variant_unref (result);
    }
  cookie_ = 0;
}

bool
IdleInhibitor::is_inhibited() const
{
  return cookie_ != 0;
}
