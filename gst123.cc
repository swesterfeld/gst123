/* GST123 - GStreamer based command line media player
 * Copyright (C) 2006 Stefan Westerfeld
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
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "glib-extra.h"
#include <vector>
#include <string>
#include <list>

#define GST123_VERSION "0.0.1"

using std::string;
using std::vector;
using std::list;

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
  guint bitrate;

  Tags() : timestamp (-1), bitrate (0)
  {
  }
};

static double
get_time ()
{
  timeval tv;
  gettimeofday (&tv, 0);

  return double(tv.tv_sec) + double(tv.tv_usec) * (1.0 / 1000000.0);
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

struct Player
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
	  if (tags.codec != "" || tags.bitrate != 0)
	    printf ("%s %s%.1f kbit/s\n",
	                          make_n_char_string ("Codec   : " + tags.codec, cols / 2 - 1).c_str(),  
	                                             ("Bitrate : "), tags.bitrate / 1000.0);
	  printf ("\n");
	  reset_tags (KEEP_CODEC_TAGS);
	}
  }

  void
  overwrite_time_display ()
  {
    for (int i = 0; i < cols; i++)
      printf (" ");
    printf ("\r");
  }

  void
  play_next()
  {
    reset_tags (RESET_ALL_TAGS);

    if (options.shuffle)
      play_position = g_random_int_range (0, uris.size());

    if (play_position < uris.size())
      {
	string uri = uris[play_position++];

	overwrite_time_display ();
	printf ("\nPlaying %s\n", uri.c_str());

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
  quit()
  {
    overwrite_time_display ();

    gst_element_set_state (playbin, GST_STATE_NULL);
    if (loop)
      g_main_loop_quit (loop);
  }

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

      player.quit();
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
      g_print ("\rTime: %02lu:%02lu.%02lu", tv_pos.tv_sec / 60, tv_pos.tv_sec % 60, tv_pos.tv_usec / 10000);
      if (len > 0)   /* streams (i.e. http) have len == -1 */
	g_print (" of %02lu:%02lu.%02lu", tv_len.tv_sec / 60, tv_len.tv_sec % 60, tv_len.tv_usec / 10000);
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
	  printf ("%s %s\n", program_name.c_str(), GST123_VERSION);
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


gint
main (gint   argc,
      gchar *argv[])
{
  Player player;

  /* init GStreamer */
  gst_init (&argc, &argv);
  player.loop = g_main_loop_new (NULL, FALSE);

  options.parse (&argc, &argv);

  /* set up */
  for (int i = 1; i < argc; i++)
    player.add_uri (argv[i]);

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

      char uri[1024];
      while (fgets (uri, 1024, f))
	{
	  int l = strlen (uri);
	  while (l && (uri[l-1] == '\n' || uri[l-1] == '\r'))
	    uri[--l] = 0;
	  player.add_uri (uri);
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
  player.playbin = gst_element_factory_make ("playbin", "play");
  gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (player.playbin)), my_bus_callback, &player);
  g_timeout_add (130, (GSourceFunc) cb_print_position, &player);
  signal (SIGINT, sigint_handler);
  g_usignal_add (SIGINT, sigint_usr_code, &player);
  player.play_next();

  /* now run */
  g_main_loop_run (player.loop);

  /* also clean up */
  gst_element_set_state (player.playbin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (player.playbin));

  return 0;
}
