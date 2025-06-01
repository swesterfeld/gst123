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
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "glib-extra.h"
#include "config.h"
#include "terminal.h"
#include "gtkinterface.h"
#include "options.h"
#include "playlist.h"
#include "visualization.h"
#include "msg.h"
#include "typefinder.h"
#include "compat.h"
#include "utils.h"
#include <vector>
#include <string>
#include <list>
#include <iostream>

using std::string;
using std::vector;
using std::list;
using std::swap;

using namespace Gst123;

static Terminal     terminal;
static GtkInterface gtk_interface;

struct Tags
{
  double timestamp;
  string title;
  string artist;
  string album;
  string date;
  string comment;
  string genre;
  string codec;
  string vcodec;
  guint bitrate;

  Tags() : timestamp (-1), bitrate (0)
  {
  }
};

static int
get_columns()
{
  int   result = 80; /* default */

  FILE *cols = popen ("tput cols", "r");
  char  col_buffer[50];
  char *line = fgets (col_buffer, 50, cols);
  if (line)
    {
      int c = atoi (col_buffer);
      if (c > 30)
        result = c;
    }
  pclose (cols);

  return result;
}

static string
get_basename (const string& path)
{
  char *basename_c = g_path_get_basename (path.c_str());
  string result = basename_c;
  g_free (basename_c);

  return result;
}

void
force_aspect_ratio (GstElement *element, gpointer userdata)
{
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (element)), "force-aspect-ratio"))
    g_object_set (G_OBJECT (element), "force-aspect-ratio", TRUE, NULL);
}

static bool
filename2uri (string& uri)
{
  GError *err = NULL;

  char *xuri = g_filename_to_uri (uri.c_str(), NULL, &err);
  uri = xuri;
  g_free (xuri);

  if (err != NULL)
    {
      g_critical ("Unable to convert to URI: %s", err->message);
      g_error_free (err);

      return false;
    }
  return true;
}

static string
uri2filename (const string& uri)
{
  GError *err = NULL;
  char *xfilename = g_filename_from_uri (uri.c_str(), NULL, &err);

  if (err != NULL)
    {
      g_critical ("Unable to convert from URI: %s", err->message);
      g_error_free (err);
    }

  if (xfilename)
    {
      string filename = xfilename;
      g_free (xfilename);

      return filename;
    }
  else
    {
      return ""; // fail
    }
}

Options options;

struct Player : public KeyHandler
{
  vector<string> uris;

  GstElement   *playbin;
  GMainLoop    *loop;

  guint         play_position;
  int           cols;
  Tags          tags;
  GstState      last_state;
  string        old_tag_str;

  double        playback_rate;
  double        playback_rate_step;

  enum
  {
    KEEP_CODEC_TAGS,
    RESET_ALL_TAGS
  };
  void
  reset_tags (int which_tags)
  {
    Tags old_tags = tags;
    tags = Tags();

    if (which_tags == KEEP_CODEC_TAGS)		/* otherwise: RESET_ALL_TAGS */
      {
	tags.codec    = old_tags.codec;
	tags.vcodec   = old_tags.vcodec;
	tags.bitrate  = old_tags.bitrate;
      }
    else
      {
        old_tag_str = "";
      }
  }

  void
  add_uri (string uri)
  {
    if (!gst_uri_is_valid (uri.c_str()))
      {
	if (!g_path_is_absolute (uri.c_str()))
	  uri = g_get_current_dir() + string (G_DIR_SEPARATOR + uri);

        if (!filename2uri (uri))
          return;
      }
    uris.push_back (uri);
  }

  void
  set_subtitle (string uri)
  {
    if (!gst_uri_is_valid (uri.c_str()))
      {
        if (!g_path_is_absolute (uri.c_str()))
          uri = g_get_current_dir() + string (G_DIR_SEPARATOR + uri);

        if (!filename2uri (uri))
          return;
      }
    g_object_set (G_OBJECT (playbin), "suburi", uri.c_str(), NULL);
  }

  string
  make_n_char_string (string s, guint n)
  {
    if (s.size() > n)
      s.resize (n);
    else
      {
	while (s.size() < n)
	  s += " ";
      }
    for (string::iterator si = s.begin(); si != s.end(); si++)
      if (*si == '\r' || *si == '\n' || *si == '\t')
	*si = ' ';
    return s;
  }
  string
  format_tags()
  {
    string tag_str;

    if (tags.title != "" || tags.artist != "")
      tag_str += make_n_char_string ("Title   : " + tags.title, cols / 2 - 1) + " " +
                 make_n_char_string ("Artist  : " + tags.artist, cols / 2 - 1) + "\n";

    if (tags.album != "" || tags.genre != "")
      tag_str += make_n_char_string ("Album   : " + tags.album, cols / 2 - 1) + " " +
	         make_n_char_string ("Genre   : " + tags.genre, cols / 2 - 1) + "\n";

    if (tags.comment != "" || tags.date != "")
      tag_str += make_n_char_string ("Comment : " + tags.comment, cols / 2 - 1) + " " +
	         make_n_char_string ("Date    : " + tags.date, cols / 2 - 1) + "\n";

    if (tags.codec != "" || tags.vcodec != "")
      tag_str += make_n_char_string ("Codec   : " + tags.codec + " (audio) " + ((tags.vcodec != "") ? tags.vcodec + " (video)":""), cols / 2 - 1) + "\n";

    return tag_str;
  }

  void
  display_tags()
  {
    if (tags.timestamp > 0)
      if (get_time() - tags.timestamp > 0.5) /* allows us to wait a bit for more tags */
	{
          /* we only display new tags if any of the tags changed */
          string tag_str = format_tags();
          if (tag_str != old_tag_str)
            {
	      overwrite_time_display();
	      Msg::print ("\n%s\n", tag_str.c_str());
              old_tag_str = tag_str;
            }

	  reset_tags (KEEP_CODEC_TAGS);
	}
  }

  void
  overwrite_time_display()
  {
    for (int i = 0; i < cols; i++)
      Msg::print (" ");
    Msg::print ("\r");
  }

  void
  remove_current_uri()
  {
    assert (play_position > 0);

    play_position--;
    uris.erase (uris.begin() + play_position);
  }

  bool
  is_image_file (const string& uri)
  {
    if (uri.substr (0, 5) == "file:")
      {
        /* convert uri "file:///foo/bar" to path "/foo/bar" */
        string filename = uri2filename (uri);
        if (filename != "")
          {
            TypeFinder tf (filename);
            if (tf.type() == "image")
              return true;
          }
      }
    return false;
  }

  // decode filename from uri to normal string
  string
  url_decode (const string& str)
  {
    if (str.substr (0, 5) == "file:")
      return uri2filename (str);

    string ret;

    for (size_t i = 0; i < str.length(); i++)
      {
        if (str[i] != '%')
          {
            ret += str[i];
          }
        else
          {
            ret += strtol (str.substr(i + 1, 2).c_str(), nullptr, 16); /* hex string -> char */
            i += 2;
        }
      }
    return ret;
  }

  void
  play_next()
  {
    reset_tags (RESET_ALL_TAGS);

    for (;;)
      {
        if (play_position == uris.size() && options.repeat)
          {
            if (uris.empty())
              {
                Msg::print ("No files remaining in playlist.\n");
                quit();
              }
            play_position = 0;
          }
        if (options.shuffle && play_position == 0)
          {
            // Fisher–Yates shuffle
            for (guint i = 0; i < uris.size(); i++)
              {
                guint j = g_random_int_range (i, uris.size());
                swap (uris[i], uris[j]);
              }
          }
        if (play_position < uris.size())
          {
            string uri = uris[play_position++];

            overwrite_time_display();

            if (is_image_file (uri))
              {
                Msg::print ("\nSkipping image %s\n", uri.c_str());

                remove_current_uri();
              }
            else
              {
                Msg::print ("\nPlaying %s\n", url_decode (uri).c_str());

                gtk_interface.set_title (get_basename (uri));

                gst_element_set_state (playbin, GST_STATE_NULL);
                g_object_set (G_OBJECT (playbin), "uri", uri.c_str(), NULL);
                if (!options.subtitle)
                  {
                    string suburi = guess_subtitle (uri);
                    if (!suburi.empty())
                      g_object_set (G_OBJECT (playbin), "suburi", suburi.c_str(), NULL);
                    else
                      g_object_set (G_OBJECT (playbin), "suburi", NULL, NULL);
                  }
                gst_element_set_state (playbin, GST_STATE_PLAYING);

                if (options.skip > 0)
                  {
                    // block until state changed and seek to skip position
                    gst_element_get_state (playbin, NULL, NULL, GST_CLOCK_TIME_NONE);
                    seek (options.skip * GST_SECOND);
                  }
                return; // -> done
              }
          }
        else
          {
            quit();
            return; // -> done
          }
      }
  }

  void
  play_prev()
  {
    reset_tags (RESET_ALL_TAGS);

    for (;;)
      {
        if (uris.size())
          {
            if (play_position == 1)
              --play_position;

            if (!play_position)
              {
                if (options.repeat)
                  play_position = uris.size() - 1;
              }
            else
                play_position -= 2;

            string uri = uris[play_position++];

            overwrite_time_display();

            if (is_image_file (uri))
              {
                Msg::print ("\nSkipping image %s\n", uri.c_str());

                remove_current_uri();
              }
            else
              {
                Msg::print ("\nPlaying %s\n", url_decode (uri).c_str());

                gtk_interface.set_title (get_basename (uri));

                gst_element_set_state (playbin, GST_STATE_NULL);
                g_object_set (G_OBJECT (playbin), "uri", uri.c_str(), NULL);
                if (!options.subtitle)
                  {
                    string suburi = guess_subtitle (uri);
                    if (!suburi.empty())
                      g_object_set (G_OBJECT (playbin), "suburi", suburi.c_str(), NULL);
                    else
                      g_object_set (G_OBJECT (playbin), "suburi", NULL, NULL);
                  }
                gst_element_set_state (playbin, GST_STATE_PLAYING);

                if (options.skip > 0)
                  {
                    // block until state changed and seek to skip position
                    gst_element_get_state (playbin, NULL, NULL, GST_CLOCK_TIME_NONE);
                    seek (options.skip * GST_SECOND);
                  }
                return; // -> done
              }
          }
        else
          {
            quit();
            return; // -> done
          }
      }
  }

  string
  guess_subtitle (string uri)
  {
    string suburi = uri;
    unsigned extpos = suburi.rfind ('.'); // File extension position

    if (extpos < suburi.length() - 1)
      suburi.replace (extpos+1, suburi.npos, "srt");
    else
      suburi = "";

    return suburi;
  }

  void
  seek (gint64 new_pos)
  {
//    overwrite_time_display();

    // * seekflag:
    //   GST_SEEK_FLAG_NONE     no flag
    //   GST_SEEK_FLAG_FLUSH    flush pipeline
    //   GST_SEEK_FLAG_ACCURATE accurate position is requested, this might be considerably slower for some formats.
    //   GST_SEEK_FLAG_KEY_UNIT seek to the nearest keyframe. This might be faster but less accurate.
    //   GST_SEEK_FLAG_SEGMENT  perform a segment seek.
    //   GST_SEEK_FLAG_SKIP     when doing fast foward or fast reverse
    //   playback, allow elements to skip frames instead of generating all
    //   frames. Since 0.10.22.
    // * seek position: multiply with GST_SECOND to convert seconds to nanoseconds or with
    //   GST_MSECOND to convert milliseconds to nanoseconds.

    if (new_pos < 0)
      new_pos = 0;

    // Use *_SET and a position for both start and stop, otherwise quick
    // changes (forward, backward, forward), can leave stop_pos as the reverse
    // start_pos and once playing forward reaches that point playback stops.
    gint64 start_pos;
    gint64 stop_pos;
    if (playback_rate >= 0)
      {
        start_pos = new_pos;
        stop_pos = GST_CLOCK_TIME_NONE;
      }
    else
      {
        // when playing in reverse it is from stop to start
        start_pos = 0;
        stop_pos = new_pos;
      }
    gst_element_seek (playbin, playback_rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                      GST_SEEK_TYPE_SET, start_pos, GST_SEEK_TYPE_SET, stop_pos);
  }

  void
  relative_seek (double displacement)
  {
    gint64 cur_pos;
    Compat::element_query_position (playbin, GST_FORMAT_TIME, &cur_pos);

    double new_pos_sec = cur_pos * (1.0 / GST_SECOND) + displacement;
    seek (new_pos_sec * GST_SECOND);
  }

  void
  set_playback_rate (double rate)
  {
    playback_rate = rate;
    Msg::update_status ("Playback Rate: %.2fx", playback_rate);

    relative_seek (0);
  }

  void
  toggle_pause()
  {
    if (last_state == GST_STATE_PAUSED) {
      gst_element_set_state (playbin, GST_STATE_PLAYING);
    }
    else if (last_state == GST_STATE_PLAYING) {
      gst_element_set_state (playbin, GST_STATE_PAUSED);
    }
  }

  void
  set_volume (gdouble volume_change)
  {
    gdouble cur_volume;
    g_object_get (G_OBJECT (playbin), "volume", &cur_volume, NULL);
    cur_volume += volume_change;

    Msg::update_status ("Volume: %4.1f%%", cur_volume * 100);

    if ((cur_volume >= 0) && (cur_volume <= 10))
      g_object_set (G_OBJECT (playbin), "volume", cur_volume, NULL);
  }

  void
  set_alpha (double alpha_change)
  {
    gtk_interface.set_opacity (alpha_change);
  }

  void
  mute_unmute()
  {
    gboolean mute;
    g_object_get (G_OBJECT (playbin), "mute", &mute, NULL);
    g_object_set (G_OBJECT (playbin), "mute", !mute, NULL);
  }

  void
  toggle_fullscreen()
  {
    gtk_interface.toggle_fullscreen();
  }

  void
  toggle_subtitle()
  {
    int flags;
    g_object_get (G_OBJECT (playbin), "flags", &flags, NULL);
    g_object_set (G_OBJECT (playbin), "flags", flags ^ 0x4, NULL);

    Msg::update_status ("Subtitles: %s", (flags & 0x4) ? "off" : "on");
  }

  void
  normal_size()
  {
    gtk_interface.normal_size();
  }

  void
  quit()
  {
    // End with a newline to preserve the time so the user knows where they
    // left off.
    Msg::print ("\n\n");

    gst_element_set_state (playbin, GST_STATE_NULL);
    if (loop)
      g_main_loop_quit (loop);
  }

  void process_input (int key);
  void print_keyboard_help();
  void add_uri_or_directory (const string& name);

  Player() : playbin (0), loop(0), play_position (0)
  {
    playback_rate = 1.0;
    playback_rate_step = pow (2, 1.0 / 7); // approximately 10%, but 7 steps will make playback rate double
    cols = get_columns();
  }
};

static void
collect_tags (const GstTagList *tag_list,
              const gchar *tag,
	      gpointer user_data)
{
  Tags& tags = *(Tags *) user_data;
  char *value;
  if (strcmp (tag, GST_TAG_TITLE) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_TITLE, &value))
    tags.title = value;
  if (strcmp (tag, GST_TAG_ARTIST) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_ARTIST, &value))
    tags.artist = value;
  if (strcmp (tag, GST_TAG_ALBUM) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_ALBUM, &value))
    tags.album = value;
  if (strcmp (tag, GST_TAG_GENRE) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_GENRE, &value))
    tags.genre = value;
  if (strcmp (tag, GST_TAG_COMMENT) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_COMMENT, &value))
    tags.comment = value;
  if (strcmp (tag, GST_TAG_AUDIO_CODEC) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_AUDIO_CODEC, &value))
    tags.codec = value;
  if (strcmp (tag, GST_TAG_BITRATE) == 0)
    gst_tag_list_get_uint (tag_list, GST_TAG_BITRATE, &tags.bitrate);
  if (strcmp (tag, GST_TAG_VIDEO_CODEC) == 0 && gst_tag_list_get_string (tag_list, GST_TAG_VIDEO_CODEC, &value))
    tags.vcodec = value;

  if (strcmp (tag, GST_TAG_DATE) == 0)
    {
      GDate *date = NULL;
      gst_tag_list_get_date (tag_list, GST_TAG_DATE, &date);

      char outstr[200];
      if (g_date_strftime (outstr, sizeof (outstr), "%Y", date))
	tags.date = outstr;
      g_date_free (date);
    }
}

class IdleResizeWindow
{
  int     width, height;

public:
  IdleResizeWindow (int width, int height) :
    width (width),
    height (height)
  {
  }

  static gboolean
  callback (gpointer *data)
  {
    IdleResizeWindow *self = (IdleResizeWindow *) data;
    gtk_interface.resize (self->width, self->height);
    delete self;

    /* do not call me again */
    return FALSE;
  }
};

static void
caps_set_cb (GObject *pad, GParamSpec *pspec, Player* player)
{
  // this callback doesn't occur in main thread

  if (GstCaps *caps = Compat::pad_get_current_caps (GST_PAD (pad)))
    {
      int width, height;

      if (Compat::video_get_size (GST_PAD (pad), &width, &height))
        {
          // resize window to match video size (must run in main thread, so we use an idle handler)

          g_idle_add ((GSourceFunc) IdleResizeWindow::callback,
                      new IdleResizeWindow (width, height));
        }
      gst_caps_unref (caps);
    }
}

static void
collect_element (GstElement *element,
                 gpointer list_ptr)
{
  /* seems that if we use push_front, the pipeline gets displayed in the
   * right order; but I don't know if thats a guarantee of an accident
   */
  list<GstElement *>& elements = *(list<GstElement*> *)list_ptr;
  gst_object_ref (element);
  elements.push_front (element);
}

static string
collect_print_elements (GstElement *parent, const list<GstElement*>& elements)
{
  string child_elements;
  for (list<GstElement *>::const_iterator i = elements.begin(); i != elements.end(); i++)
    {
      GstElement *child = *i;
      if (GST_ELEMENT_PARENT (child) == parent)
	{
	  child_elements += " ";
	  child_elements += collect_print_elements (child, elements);
	}
    }

  string element_name = G_OBJECT_TYPE_NAME (parent);
  if (child_elements != "")
    return element_name + " (" + child_elements + " )";
  else
    return element_name;
}

static GstBusSyncReply
my_sync_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ELEMENT:
      {
        if (Compat::is_video_overlay_prepare_window_handle_message (message) && gtk_interface.init_ok())
          {
            Compat::video_overlay_set_window_handle (message, gtk_interface.window_xid_nolock());
          }
      }
      break;
    default:
      /* unhandled message */
      break;
  }

  return GST_BUS_PASS;
}

static gboolean
my_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  Player& player = *(Player *) data;
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR: {
      GError *err = NULL;
      gchar *debug = NULL;

      gst_message_parse_error (message, &err, &debug);
      player.overwrite_time_display();
      g_print ("Error: %s\n", err ? err->message : "<NULL Error>");
      g_error_free (err);
      g_free (debug);

      g_print ("=> file cannot be played and will be removed from playlist\n\n");
      player.remove_current_uri();
      player.play_next();
      break;
    }
    case GST_MESSAGE_EOS:
      /* end-of-stream */
      player.play_next();
      break;
    case GST_MESSAGE_TAG:
      {
	GstTagList *tag_list = NULL;
	gst_message_parse_tag (message, &tag_list);
	gst_tag_list_foreach (tag_list, collect_tags, &player.tags);
	gst_tag_list_free (tag_list);
	player.tags.timestamp = get_time();
      }
      break;
    case GST_MESSAGE_STATE_CHANGED:
      {
	GstState state = GST_STATE (player.playbin);
	if (options.verbose && player.last_state != GST_STATE_PLAYING && state == GST_STATE_PLAYING)
	  {
	    list<GstElement *> elements;

	    GstIterator *iterator = gst_bin_iterate_recurse (GST_BIN (player.playbin));
	    Compat::iterator_foreach (iterator, collect_element, &elements);

	    string print_elements = collect_print_elements (GST_ELEMENT (player.playbin), elements);
	    player.overwrite_time_display();
	    Msg::print ("\ngstreamer pipeline contains: %s\n", print_elements.c_str());

            list<GstElement *>::iterator it;
            for (it = elements.begin(); it != elements.end(); it++)
              gst_object_unref (*it);
	  }
	player.last_state = state;
      }
      break;
    default:
      /* unhandled message */
      break;
  }

  if (Compat::is_stream_start_message (message))
    {
      // try to figure out the video size
      GstElement *videosink = NULL;
      g_object_get (G_OBJECT (player.playbin), "video-sink", &videosink, NULL);
      if (videosink && !options.novideo)
        {
          if (GST_IS_BIN (videosink))
            {
              // Find an sink element that has "force-aspect-ratio" property & set it
              // to TRUE:
              GstIterator *iterator = gst_bin_iterate_sinks (GST_BIN (videosink));
              Compat::iterator_foreach (iterator, force_aspect_ratio, NULL);
            }
          else
            {
              force_aspect_ratio (videosink, NULL);
            }

          if (GstPad* pad = gst_element_get_static_pad (videosink, "sink"))
            {
              if (GstCaps *caps = Compat::pad_get_current_caps (pad))
                {
                  caps_set_cb (G_OBJECT (pad), NULL, &player);
                  gst_caps_unref (caps);
                }
              /* connect videosink / videopad "notify::caps" signal
               *
               * if we did this before (while playing another video file), skip
               * this step in order to avoid handling the same signal more than
               * once
               */
              if (!g_signal_handler_find (pad,
                       /* mask      */ GSignalMatchType (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),
                       /* signal_id */ 0,
                       /* detail    */ 0,
                       /* closure   */ 0,
                       /* func      */ (void *) caps_set_cb,
                       /* data      */ &player))
                {
                  g_signal_connect (pad, "notify::caps", G_CALLBACK (caps_set_cb), &player);
                }
              gst_object_unref (GST_OBJECT (pad));
            }
          gst_object_unref (GST_OBJECT (videosink));
        }
      /* show window if necessary (number of video streams > 0 || visualization) */
      int n_video = 0;
      GstElement *vis_plugin = NULL;
      g_object_get (player.playbin, "n-video", &n_video, NULL);
      g_object_get (player.playbin, "vis-plugin", &vis_plugin, NULL);

      if (gtk_interface.init_ok())
        {
          if (n_video || vis_plugin)
            gtk_interface.show();
          else
            gtk_interface.hide();
        }
    }

  /* remove message from the queue */
  return TRUE;
}

static void
sigint_handler (int signum)
{
  g_usignal_notify (signum);
}

static gboolean
sigint_usr_code (gint8    usignal,
                 gpointer data)
{
  static double last_int = 0;

  Player& player = *(Player *)data;
  double now = get_time();
  double elapsed_ms = (now - last_int) * 1000;
  if (elapsed_ms > 0 && elapsed_ms < 500)
    player.quit();
  else
    player.play_next();
  last_int = now;

  return TRUE;
}

static gboolean
cb_print_position (gpointer *data)
{
  Player& player = *(Player *)data;
  gint64 pos, len;

  player.display_tags();

  if (Compat::element_query_position (player.playbin, GST_FORMAT_TIME, &pos) &&
      Compat::element_query_duration (player.playbin, GST_FORMAT_TIME, &len))
    {
      guint pos_ms = (pos % GST_SECOND) / 1000000;
      guint len_ms = (len % GST_SECOND) / 1000000;
      guint pos_sec = pos / GST_SECOND;
      guint len_sec = len / GST_SECOND;
      guint pos_min = pos_sec / 60;
      guint len_min = len_sec / 60;

      player.overwrite_time_display();
      Msg::print ("\rTime: %01u:%02u:%02u.%02u", pos_min / 60, pos_min % 60, pos_sec % 60, pos_ms / 10);
      if (len > 0)   /* streams (i.e. http) have len == -1 */
        Msg::print (" of %01u:%02u:%02u.%02u", len_min / 60, len_min % 60, len_sec % 60, len_ms / 10);

      string message = Msg::status();
      if (message != "")
        {
          Msg::print (" | %s", message.c_str());
        }
      else
        {
          /* only print bitrate if no status message needs to be shown, in
           * order to avoid too long output lines
           */
          if (player.tags.bitrate > 0)
            Msg::print (" | Bitrate: %.1f kbit/sec", player.tags.bitrate / 1000.);
        }

      string status, blanks;
      // Print [MUTED] if sound is muted:
      gboolean mute;
      g_object_get (G_OBJECT (player.playbin), "mute", &mute, NULL);

      if (mute)
        status += " [MUTED]";
      else
        blanks += "        ";

      // Print [PAUSED] if paused:
      bool pause = (player.last_state == GST_STATE_PAUSED);

      if (pause)
        status += " [PAUSED]";
      else
        blanks += "         ";
      Msg::print ("%s%s\r", status.c_str(), blanks.c_str());
      Msg::flush();
    }

  /* call me again */
  return TRUE;
}

static gboolean
idle_start_player (gpointer *data)
{
  Player& player = *(Player *)data;

  player.play_next();

  /* do not call me again */
  return FALSE;
}


enum FileInfo { FI_DIR, FI_REG, FI_OTHER, FI_ERROR };

static inline FileInfo
file_info (const string& path)
{
  struct stat st;

  if (lstat (path.c_str(), &st) == 0)
    {
      if (S_ISDIR (st.st_mode))
        return FI_DIR;
      else if (S_ISREG (st.st_mode))
        return FI_REG;
      else
        {
          if (stat (path.c_str(), &st) == 0 && S_ISREG (st.st_mode))
            return FI_REG;
          else
            return FI_OTHER;
        }
    }
  return FI_ERROR;
}

static vector<string>
crawl (const string& path)
{
  vector<string> results;

  GDir *dir = g_dir_open (path.c_str(), 0, NULL);
  if (dir)
    {
      const char *name;

      while ((name = g_dir_read_name (dir)))
        {
          char *full_name = g_build_filename (path.c_str(), name, NULL);
          FileInfo finfo = file_info (full_name);

          if (finfo == FI_DIR)
            {
              vector<string> subdir_files = crawl (full_name);
              results.insert (results.end(), subdir_files.begin(), subdir_files.end());
            }
          else if (finfo == FI_REG)
            {
              results.push_back (full_name);
            }
          g_free (full_name);
        }
      g_dir_close (dir);
    }
  return results;
}

/*
 * handle user input
 */
void
Player::process_input (int key)
{
  switch (key)
    {
      case KEY_HANDLER_RIGHT:
        relative_seek (10);
        break;
      case KEY_HANDLER_LEFT:
        relative_seek (-10);
        break;
      case KEY_HANDLER_UP:
        relative_seek (60);
        break;
      case KEY_HANDLER_DOWN:
        relative_seek (-60);
        break;
      case KEY_HANDLER_PAGE_UP:
        relative_seek (600);
        break;
      case KEY_HANDLER_PAGE_DOWN:
        relative_seek (-600);
        break;
      case 'Q':
      case 'q':
        quit();
        break;
      case ' ':
        toggle_pause();
        break;
      case '+':
        set_volume (0.1);
        break;
      case '-':
        set_volume (-0.1);
        break;
      case 'A':
        set_alpha (0.1);
        break;
      case 'a':
        set_alpha (-0.1);
        break;
      case 'M':
      case 'm':
        mute_unmute();
        break;
      case 'F':
      case 'f':
        toggle_fullscreen();
        break;
      case 'S':
      case 's':
        toggle_subtitle();
        break;
      case 'N':
      case 'n':
        play_next();
        break;
      case 'P':
      case 'p':
        play_prev();
        break;
      case '1':
        normal_size();
        break;
      case 'r':
        set_playback_rate (playback_rate * -1);
        break;
      case KEY_HANDLER_BACKSPACE:
        set_playback_rate (1);
        break;
      case '[':
        set_playback_rate (playback_rate / playback_rate_step);
        break;
      case ']':
        set_playback_rate (playback_rate * playback_rate_step);
        break;
      case '{':
        set_playback_rate (playback_rate / 2);
        break;
      case '}':
        set_playback_rate (playback_rate * 2);
        break;
      case '?':
      case 'h':
        print_keyboard_help();
        break;
    }
}

void
Player::print_keyboard_help()
{
  overwrite_time_display();

  printf ("\n\n");
  printf ("==================== gst123 keyboard commands =======================\n");
  printf ("   cursor left/right    -     seek 10 seconds backwards/forwards\n");
  printf ("   cursor down/up       -     seek 1  minute  backwards/forwards\n");
  printf ("   page down/up         -     seek 10 minute  backwards/forwards\n");
  printf ("   space                -     toggle pause\n");
  printf ("   +/-                  -     increase/decrease volume by 10%%\n");
  printf ("   m                    -     toggle mute/unmute\n");
  printf ("   f                    -     toggle fullscreen (only for videos)\n");
  printf ("   1                    -     normal video size (only for videos)\n");
  printf ("   A/a                  -     increase/decrease opacity by 10%% (only for videos)\n");
  printf ("   s                    -     toggle subtitles  (only for videos)\n");
  printf ("   r                    -     reverse playback\n");
  printf ("   [ ]                  -     playback rate 10%% faster/slower\n");
  printf ("   { }                  -     playback rate 2x faster/slower\n");
  printf ("   Backspace            -     playback rate 1x\n");
  printf ("   n                    -     play next file\n");
  printf ("   p                    -     play prev file\n");
  printf ("   q                    -     quit gst123\n");
  printf ("   ?|h                  -     this help\n");
  printf ("=====================================================================\n");
  printf ("\n\n");
}

void
Player::add_uri_or_directory (const string& name)
{
  if (file_info (name) == FI_DIR)          // => play all files in this dir
    {
      vector<string> uris = crawl (name);
      for (vector<string>::const_iterator ui = uris.begin(); ui != uris.end(); ui++)
        add_uri (*ui);
    }
  else
    {
      add_uri (name);
    }
}

gint
main (gint   argc,
      gchar *argv[])
{
  Player player;

  if (XInitThreads() == 0)
    {
      fprintf (stderr, "%s: Failed to initialize Xlib support for concurrent threads (XInitThreads() failed).\n", argv[0]);
      return -1;
    }

  /* Setup options */
  options.parse (argc, argv);

  /* init GStreamer */
  gst_init (&argc, &argv);
  gtk_interface.init (&argc, &argv, &player);

  if (options.print_visualization_list)
    {
      Visualization::print_visualization_list();
      return 0;
    }

  player.loop = g_main_loop_new (NULL, FALSE);

  /* set up */
  if (options.uris)
    {
      for (int i = 0; options.uris[i]; i++)
        player.add_uri_or_directory (options.uris[i]);
    }

  for (list<string>::iterator pi = options.playlists.begin(); pi != options.playlists.end(); pi++)
    {
      Playlist pls (*pi);

      if (!pls.is_valid())
        {
          std::cerr << "Could not load playlist " << *pi << std::endl;
          return -1;
        }

      char *playlist_dirname = g_path_get_dirname (pi->c_str());
      for (unsigned int i = 0; i < pls.size(); i++)
        {
          if ((pls[i].find (":") == string::npos) && !g_path_is_absolute (pls[i].c_str()))
            {
              char *filename = g_build_filename (playlist_dirname, pls[i].c_str(), NULL);
              player.add_uri_or_directory (filename);
              g_free (filename);
            }
          else
            player.add_uri_or_directory (pls[i].c_str());
        }
      g_free (playlist_dirname);
    }
  /* make sure we have a URI */
  if (player.uris.empty())
    {
      /* Don't print usage if a playlist was provided */
      if (!options.playlists.size())
        printf ("%s", options.usage.c_str());
      return -1;
    }
  player.playbin = Compat::create_playbin ("play");
  if (options.novideo)
    {
      GstElement *fakesink = gst_element_factory_make ("fakesink", "novid");
      g_object_set (G_OBJECT (player.playbin), "video-sink", fakesink, NULL);
    }
  if (options.visualization)
    {
      if (!Visualization::setup (player.playbin))
        {
          printf ("visualization plugin %s not found\n", options.visualization);
          return -1;
        }
    }
  if (options.audio_output)
    {
      char *audio_driver = strtok (options.audio_output, "=");
      char *audio_device = strtok (NULL, "");
      GstElement *audio_sink = NULL;
      if (audio_driver)
        {
          if (strcmp (audio_driver, "alsa") == 0)
            audio_sink = gst_element_factory_make ("alsasink", "alsaaudioout");
          else if (strcmp (audio_driver, "oss") == 0)
            audio_sink = gst_element_factory_make ("osssink", "ossaudioout");
          else if (strcmp (audio_driver, "jack") == 0)
            audio_sink = gst_element_factory_make ("jackaudiosink", "jackaudioout");
          else if (strcmp (audio_driver, "pulse") == 0)
            audio_sink = gst_element_factory_make ("pulsesink", "pulseaudioout");
          else if (strcmp (audio_driver, "none") == 0)
            audio_sink = gst_element_factory_make ("fakesink", "fakeaudioout");
          else
            {
              printf ("%s: unknown audio driver %s\n", argv[0], audio_driver);
              return -1;
            }
        }
      if (audio_sink)
        {
          if (audio_device)
            g_object_set (G_OBJECT (audio_sink), "device", audio_device, NULL);
          g_object_set (G_OBJECT (player.playbin), "audio-sink", audio_sink, NULL);
        }
    }
  if (options.initial_volume >= 0)
    {
      g_object_set (G_OBJECT (player.playbin), "volume", options.initial_volume / 100, NULL);
    }
  if (options.subtitle)
    {
      player.set_subtitle (options.subtitle);
    }

  Compat::setup_bus_callbacks (GST_PIPELINE (player.playbin), my_sync_bus_callback, my_bus_callback, &player);

  g_timeout_add (130, (GSourceFunc) cb_print_position, &player);
  g_idle_add ((GSourceFunc) idle_start_player, &player);
  signal (SIGINT, sigint_handler);
  g_usignal_add (SIGINT, sigint_usr_code, &player);

  /* now run */
  terminal.init (player.loop, &player);
  g_main_loop_run (player.loop);
  terminal.end();
  gtk_interface.end();

  /* also clean up */
  gst_element_set_state (player.playbin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (player.playbin));

  return 0;
}
