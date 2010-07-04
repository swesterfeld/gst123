/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006-2010 Stefan Westerfeld
 * Copyright (C) 2010 أحمد المحمودي (Ahmed El-Mahmoudy)
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
#include <assert.h>
#include <gst/gst.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>

#include "config.h"
#include "options.h"
#include "gtkinterface.h"

Options *Options::instance = NULL;

Options::Options ()
{
  assert (!instance);
  instance = this; // singleton

  program_name = "gst123";
  shuffle = FALSE;
  verbose = FALSE;
  novideo = FALSE;
  uris = NULL;
  audio_driver = NULL;
  audio_device = NULL;
}

void
Options::parse (int argc, char **argv)
{
  GOptionContext *context = g_option_context_new ("<URI>... - Play video and audio clips");
  const GOptionEntry all_options[] = {
    {"list", '@', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_CALLBACK,
      (GOptionParseFunc*) Options::add_playlist,
      "read playlist of files and URIs from <filename>", "<filename>"},
    {"version", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
      (GOptionParseFunc*) Options::print_version, "print version", NULL },
    {"verbose", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &instance->verbose,
      "print GStreamer pipeline used to play files", NULL},
    {"shuffle", 'z', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &instance->shuffle,
      "play files in pseudo random order", NULL},
    {"novideo", 'x', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &instance->novideo,
      "do not play the video stream", NULL},
    {"audio-driver", 'a', 0, G_OPTION_ARG_STRING, &instance->audio_driver,
      "set audio driver used for audio output", NULL},
    {"audio-device", 'd', 0, G_OPTION_ARG_STRING, &instance->audio_device,
      "set audio device used for audio output", NULL},
    {G_OPTION_REMAINING, '\0', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_FILENAME_ARRAY, &instance->uris, "Movies to play", NULL},
    {NULL} /* end the list */
  };
  g_option_context_add_main_entries (context, all_options, NULL);
  g_option_context_add_group (context, gst_init_get_option_group());

  if (GtkInterface::have_x11_display())
    g_option_context_add_group (context, gtk_get_option_group (TRUE));

  GError *error = NULL;
  bool option_parse_ret = g_option_context_parse (context, &argc, &argv, &error);
  usage = g_option_context_get_help (context, TRUE, NULL);
  if (!option_parse_ret)
    {
      g_print ("%s\n%s", error->message, usage.c_str());
      g_error_free (error);
      g_option_context_free (context);
      exit (1);
    }
  g_option_context_free (context);
}

void
Options::print_version ()
{
  printf ("%s %s\n", instance->program_name.c_str(), VERSION);
  exit (0);
}

void
Options::add_playlist (const gchar *option_name, const gchar *value)
{
  instance->playlists.push_back (value);
}
