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
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "glib-extra.h"
#include "config.h"
#include "terminal.h"
#include "gtkinterface.h"
#include <vector>
#include <string>
#include <list>

using std::string;
using std::vector;
using std::list;

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

static double
get_time()
{
  timeval tv;
  gettimeofday (&tv, 0);

  return double (tv.tv_sec) + double (tv.tv_usec) * (1.0 / 1000000.0);
}

static int
get_columns()
{
  FILE *cols = popen ("tput cols", "r");
  char col_buffer[50];
  fgets (col_buffer, 50, cols);
  int c = atoi (col_buffer);
  if (c > 30)
    return c;
  else
    return 80;	/* default */
  fclose (cols);
}

void
force_aspect_ratio (gpointer element, gpointer userdata)
{
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (element)), "force-aspect-ratio"))
    g_object_set (G_OBJECT (element), "force-aspect-ratio", TRUE, NULL);
}

struct Options
{
  string	program_name; /* FIXME: what to do with that */
  bool          verbose;
  bool          shuffle;
  list<string>  playlists;

  Options ();
  void parse (int *argc_p, char **argv_p[]);
  static void print_usage ();
} options;

Options::Options ()
{
  program_name = "gst123";
  shuffle = false;
  verbose = false;
}

struct Player : public KeyHandler
{
  vector<string> uris;

  GstElement    *playbin;
  GMainLoop     *loop;

  guint          play_position;
  int            cols;
  Tags           tags;
  GstState       last_state;

  enum
  {
    KEEP_CODEC_TAGS,
    RESET_ALL_TAGS
  };
  void
  reset_tags (int which_tags)
  {
    string old_codec = tags.codec;
    guint old_bitrate = tags.bitrate;
    tags = Tags();

    if (which_tags == KEEP_CODEC_TAGS)		/* otherwise: RESET_ALL_TAGS */
      {
	tags.codec = old_codec;
	tags.bitrate = old_bitrate;
      }
  }

  void
  add_uri (string uri)
  {
    if (uri.find (':') == uri.npos)
      {
	if (!g_path_is_absolute (uri.c_str()))
	  uri = g_get_current_dir() + string (G_DIR_SEPARATOR + uri);
	uri = "file://" + uri;
      }
    uris.push_back (uri);
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

  void
  display_tags()
  {
    if (tags.timestamp > 0)
      if (get_time() - tags.timestamp > 0.5) /* allows us to wait a bit for more tags */
	{
	  overwrite_time_display();
	  printf ("\n");
	  if (tags.title != "" || tags.artist != "")
	    printf ("%s %s\n",	  make_n_char_string ("Title   : " + tags.title, cols / 2 - 1).c_str(),
	                          make_n_char_string ("Artist  : " + tags.artist, cols / 2 - 1).c_str());
	  if (tags.album != "" || tags.genre != "")
	    printf ("%s %s\n",	  make_n_char_string ("Album   : " + tags.album, cols / 2 - 1).c_str(),
	                          make_n_char_string ("Genre   : " + tags.genre, cols / 2 - 1).c_str());
	  if (tags.comment != "" || tags.date != "")
	    printf ("%s %s\n",	  make_n_char_string ("Comment : " + tags.comment, cols / 2 - 1).c_str(),
	                          make_n_char_string ("Date    : " + tags.date, cols / 2 - 1).c_str());
	  if (tags.codec != "" || tags.vcodec != "" || tags.bitrate != 0)
	    printf ("%s %s%.1f kbit/s\n",
	                          make_n_char_string ("Codec   : " + tags.codec + " (audio) " + ((tags.vcodec != "") ? tags.vcodec + " (video)":""), cols / 2 - 1).c_str(),
	                                             ("Bitrate : "), tags.bitrate / 1000.0);
	  printf ("\n");
	  reset_tags (KEEP_CODEC_TAGS);
	}
  }

  void
  overwrite_time_display()
  {
    for (int i = 0; i < cols; i++)
      printf (" ");
    printf ("\r");
  }

  void
  remove_current_uri()
  {
    play_position--;
    uris.erase (uris.begin() + play_position);
  }

  void
  play_next()
  {
    reset_tags (RESET_ALL_TAGS);

    /*
     * if we're playing an audio file, we don't need the GtkInterface, so we hide it here
     * if we're playing a video file, it will be shown again once GStreamer gets to the
     * point where it sends a "prepare-xwindow-id" message
     */
    gtk_interface.hide();

    if (options.shuffle)
      {
        if (uris.empty())
          {
            printf ("No files remaining in playlist.\n");
            quit();
          }
        else
          {
            play_position = g_random_int_range (0, uris.size());
          }
      }
    if (play_position < uris.size())
      {
	string uri = uris[play_position++];

	overwrite_time_display ();
	printf ("\nPlaying %s\n", uri.c_str());

        gtk_interface.set_title (g_basename (uri.c_str()));

	gst_element_set_state (playbin, GST_STATE_NULL);
	g_object_set (G_OBJECT (playbin), "uri", uri.c_str(), NULL);
	gst_element_set_state (playbin, GST_STATE_PLAYING);
      }
    else
      {
	/* done */
	quit();
      }
  }

  void
  seek (gint64 new_pos)
  {
//    overwrite_time_display ();

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

    gst_element_seek (playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET,
                      new_pos, GST_SEEK_TYPE_NONE,  GST_CLOCK_TIME_NONE);
  }

  void
  relative_seek (double displacement)
  {
    GstFormat fmt = GST_FORMAT_TIME;
    gint64    cur_pos;
    gst_element_query_position (playbin, &fmt, &cur_pos);

    double new_pos_sec = cur_pos * (1.0 / GST_SECOND) + displacement;
    seek (new_pos_sec * GST_SECOND);
  }

  void
  toggle_pause()
  {
//    overwrite_time_display ();

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

    overwrite_time_display();
    printf ("Volume: %4.1f%% \n", cur_volume*100);

    if ((cur_volume >= 0) && (cur_volume <= 10))
      g_object_set (G_OBJECT (playbin), "volume", cur_volume, NULL);
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
  normal_size()
  {
    GstElement *videosink;

    g_object_get (G_OBJECT (playbin), "video-sink", &videosink, NULL);
    if (videosink)
      {
        // Find an sink element that has "force-aspect-ratio" property & set it
        // to TRUE:
        GstIterator *iterator = gst_bin_iterate_sinks (GST_BIN (videosink));
        gst_iterator_foreach (iterator, force_aspect_ratio, NULL);

        if (GstPad* pad = gst_element_get_static_pad (videosink, "sink"))
          {
            gint x, y = 0;
            gst_video_get_size (GST_PAD (pad), &x, &y);
            gtk_interface.resize (x,y);
            gst_object_unref (GST_OBJECT (pad));
          }
        gst_object_unref (GST_OBJECT (videosink));
      }
  }

  void
  quit()
  {
    overwrite_time_display();

    gst_element_set_state (playbin, GST_STATE_NULL);
    if (loop)
      g_main_loop_quit (loop);
  }

  void process_input (int key);
  void print_keyboard_help();
  void add_uri_or_directory (const string& name);

  Player() : playbin (0), loop(0), play_position (0)
  {
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

static void
collect_element (gpointer element,
                 gpointer list_ptr)
{
  /* seems that if we use push_front, the pipeline gets displayed in the
   * right order; but I don't know if thats a guarantee of an accident
   */
  list<GstElement *>& elements = *(list<GstElement*> *)list_ptr;
  elements.push_front (GST_ELEMENT (element));
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

static gboolean
my_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  Player& player = *(Player *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR: {
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      player.overwrite_time_display();
      g_print ("Error: %s\n", err->message);
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
	    gst_iterator_foreach (iterator, collect_element, &elements);
	    string print_elements = collect_print_elements (GST_ELEMENT (player.playbin), elements);
	    player.overwrite_time_display();
	    printf ("\ngstreamer pipeline contains: %s\n", print_elements.c_str());
	  }
	player.last_state = state;
      }
      break;
    case GST_MESSAGE_ELEMENT:
      {
        if (gst_structure_has_name (message->structure, "prepare-xwindow-id") && gtk_interface.init_ok())
          {
            // show gtk window to display video in
            gtk_interface.show();
            gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (GST_MESSAGE_SRC (message)),
                                          GDK_WINDOW_XWINDOW (gtk_interface.window()->window));
            player.normal_size();
          }
      }
    default:
      /* unhandled message */
      break;
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
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 pos, len;

  player.display_tags();

  if (gst_element_query_position (player.playbin, &fmt, &pos) && gst_element_query_duration (player.playbin, &fmt, &len))
    {
      GTimeVal tv_pos, tv_len;
      GST_TIME_TO_TIMEVAL (pos, tv_pos);
      GST_TIME_TO_TIMEVAL (len, tv_len);

      glong pos_min = tv_pos.tv_sec / 60;
      glong len_min = tv_len.tv_sec / 60;
      g_print ("\rTime: %01lu:%02lu:%02lu.%02lu", pos_min / 60, pos_min % 60, tv_pos.tv_sec % 60, tv_pos.tv_usec / 10000);
      if (len > 0)   /* streams (i.e. http) have len == -1 */
	g_print (" of %01lu:%02lu:%02lu.%02lu", len_min / 60, len_min % 60, tv_len.tv_sec % 60, tv_len.tv_usec / 10000);

      // Print [MUTED] if sound is muted:
      gboolean mute;
      g_object_get (G_OBJECT (player.playbin), "mute", &mute, NULL);

      if (mute)
        g_print (" [MUTED]");
      else
        g_print ("        ");
      g_print ("\r");
    }

  /* call me again */
  return TRUE;
}

static bool
check_arg (uint         argc,
           char        *argv[],
           uint        *nth,
           const char  *opt,		  /* for example: --foo */
           const char **opt_arg = NULL)	  /* if foo needs an argument, pass a pointer to get the argument */
{
  g_return_val_if_fail (opt != NULL, false);
  g_return_val_if_fail (*nth < argc, false);

  const char *arg = argv[*nth];
  if (!arg)
    return false;

  uint opt_len = strlen (opt);
  if (strcmp (arg, opt) == 0)
    {
      if (opt_arg && *nth + 1 < argc)	  /* match foo option with argument: --foo bar */
        {
          argv[(*nth)++] = NULL;
          *opt_arg = argv[*nth];
          argv[*nth] = NULL;
          return true;
        }
      else if (!opt_arg)		  /* match foo option without argument: --foo */
        {
          argv[*nth] = NULL;
          return true;
        }
      /* fall through to error message */
    }
  else if (strncmp (arg, opt, opt_len) == 0 && arg[opt_len] == '=')
    {
      if (opt_arg)			  /* match foo option with argument: --foo=bar */
        {
          *opt_arg = arg + opt_len + 1;
          argv[*nth] = NULL;
          return true;
        }
      /* fall through to error message */
    }
  else
    return false;

  Options::print_usage();
  exit (1);
}

void
Options::parse (int   *argc_p,
                char **argv_p[])
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  unsigned int i, e;

  g_return_if_fail (argc >= 0);

  /*  I am tired of seeing .libs/lt-gst123 all the time,
   *  but basically this should be done (to allow renaming the binary):
   *
  if (argc && argv[0])
    program_name = argv[0];
  */

  for (i = 1; i < argc; i++)
    {
      const char *opt_arg;
      if (strcmp (argv[i], "--help") == 0 ||
          strcmp (argv[i], "-h") == 0)
	{
	  print_usage();
	  exit (0);
	}
      else if (strcmp (argv[i], "--version") == 0 || strcmp (argv[i], "-v") == 0)
	{
	  printf ("%s %s\n", program_name.c_str(), VERSION);
	  exit (0);
	}
      else if (check_arg (argc, argv, &i, "--verbose"))
	{
	  verbose = true;
	}
      else if (check_arg (argc, argv, &i, "--shuffle") || check_arg (argc, argv, &i, "-z"))
	{
	  shuffle = true;
	}
      else if (check_arg (argc, argv, &i, "--list", &opt_arg) || check_arg (argc, argv, &i, "-@", &opt_arg))
	{
	  playlists.push_back (opt_arg);
	}
    }

  /* resort argc/argv */
  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}

void
Options::print_usage ()
{
  g_printerr ("usage: %s [ <options> ] <URI> ...\n", options.program_name.c_str());
  g_printerr ("\n");
  g_printerr ("options:\n");
  g_printerr (" -h, --help                  help for %s\n", options.program_name.c_str());
  g_printerr (" -@, --list <filename>       read playlist of files and URIs from \"filename\"\n");
  g_printerr (" --version                   print version\n");
  g_printerr (" --verbose                   print GStreamer pipeline used to play files\n");
  g_printerr (" -z, --shuffle               play files in pseudo random order\n");
  g_printerr ("\n");
}

static inline bool
is_directory (const string& path)
{
  return g_file_test (path.c_str(), G_FILE_TEST_IS_DIR);
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
          if (is_directory (full_name))
            {
              vector<string> subdir_files = crawl (full_name);
              results.insert (results.end(), subdir_files.begin(), subdir_files.end());
            }
          else
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
      case 'M':
      case 'm':
        mute_unmute();
        break;
      case 'F':
      case 'f':
        toggle_fullscreen();
        break;
      case '1':
        normal_size();
        break;
      case '?':
        print_keyboard_help();
        break;
    }
}

void
Player::print_keyboard_help()
{
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
  printf ("   q                    -     quit gst123\n");
  printf ("   ?                    -     this help\n");
  printf ("=====================================================================\n");
  printf ("\n\n");
}

void
Player::add_uri_or_directory (const string& name)
{
  if (is_directory (name))          // => play all files in this dir
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

  /* init GStreamer */
  gst_init (&argc, &argv);
  gtk_interface.init (&argc, &argv, &player);

  player.loop = g_main_loop_new (NULL, FALSE);

  options.parse (&argc, &argv);

  /* set up */
  for (int i = 1; i < argc; i++)
    player.add_uri_or_directory (argv[i]);

  for (list<string>::iterator pi = options.playlists.begin(); pi != options.playlists.end(); pi++)
    {
      FILE *f = stdin;
      if (*pi != "-")
	{
	  f = fopen (pi->c_str(), "r");
	  if (!f)
	    {
	      g_printerr ("gst123: can't read playlist %s: %s\n", pi->c_str(), strerror (errno));
	      exit (1);
	    }
	}

      string playlist_dirname = g_path_get_dirname (pi->c_str());
      char uri[1024];
      while (fgets (uri, 1024, f))
	{
	  int l = strlen (uri);
	  while (l && (uri[l-1] == '\n' || uri[l-1] == '\r'))
	    uri[--l] = 0;
          if (!g_path_is_absolute (uri))
            player.add_uri_or_directory (g_build_filename (playlist_dirname.c_str(), uri, 0));
	  else
            player.add_uri_or_directory (uri);
	}

      if (*pi != "-")
	fclose (f);
    }
  /* make sure we have a URI */
  if (player.uris.empty())
    {
      options.print_usage();
      return -1;
    }
  player.playbin = gst_element_factory_make ("playbin2", "play");
  gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (player.playbin)), my_bus_callback, &player);
  g_timeout_add (130, (GSourceFunc) cb_print_position, &player);
  signal (SIGINT, sigint_handler);
  g_usignal_add (SIGINT, sigint_usr_code, &player);
  player.play_next();

  /* now run */
  terminal.init (player.loop, &player);
  g_main_loop_run (player.loop);
  terminal.end();

  /* also clean up */
  gst_element_set_state (player.playbin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (player.playbin));

  return 0;
}
