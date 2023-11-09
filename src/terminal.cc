/* GST123 - GStreamer based command line media player
 * Copyright (C) 2010 Stefan Westerfeld
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
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <term.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <vector>
#include <map>

#include "terminal.h"

using std::vector;
using std::string;
using std::map;

static char term_buffer[4096];
static char term_buffer2[4096];
static char *term_p = term_buffer2;
static struct termios tio_orig;
static Terminal *terminal_instance;

static GPollFD stdin_poll_fd = { 0, G_IO_IN, 0 };

static gboolean
stdin_prepare (GSource    *source,
               gint       *timeout)
{
  *timeout = -1;
  return FALSE;
}

static gboolean
stdin_check (GSource *source)
{
  if (stdin_poll_fd.revents & G_IO_IN)
    return TRUE;
  else
    return FALSE;
}

static void
reset_terminal ()
{
  tcsetattr(0, TCSANOW, &tio_orig);
  char *ret = tgetstr ("ke", &term_p);
  // disable keypad xmit
  if (ret)
    printf ("%s", ret);
  fflush (stdout);
}

gboolean
Terminal::stdin_dispatch (GSource    *source,
                          GSourceFunc callback,
                          gpointer    user_data)
{
  terminal_instance->read_stdin();

  int key;
  do
    {
      key = terminal_instance->getchar();

      if (key > 0)
        terminal_instance->key_handler->process_input (key);
    }
  while (key > 0);

  return TRUE;
}

void
Terminal::signal_sig_cont (int)
{
  terminal_instance->init_terminal();
}

void
Terminal::init_terminal()
{
  // configure input params of the terminal
  tcgetattr (0, &tio_orig);

  struct termios tio_new = tio_orig;
  tio_new.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
  tio_new.c_cc[VMIN] = 0;
  tio_new.c_cc[VTIME] = 1; /* 0.1 seconds */
  tcsetattr (0, TCSANOW, &tio_new);

  // enable keypad_xmit
  char *ret = tgetstr ("ks", &term_p);
  // disable keypad xmit
  if (ret)
    printf ("%s", ret);
  fflush (stdout);
}

void
Terminal::bind_key (const char *key, int handler)
{
  char *ret = tgetstr (const_cast<char *> (key), &term_p);
  if (ret)
    keys[ret] = handler;
}

void
Terminal::init (GMainLoop *loop, KeyHandler *key_handler)
{
  terminal_instance = this;
  this->key_handler = key_handler;

  const char *termtype = NULL;
  termtype = getenv ("TERM");
  if (termtype)
    terminal_type = termtype;
  else
    terminal_type = "unknown";

  tgetent (term_buffer, terminal_type.c_str());

  // initialize termios & keypad xmit
  init_terminal();
  atexit(reset_terminal);

  // initialize common keyboard escape sequences
  bind_key ("ku", KEY_HANDLER_UP);
  bind_key ("kd", KEY_HANDLER_DOWN);
  bind_key ("kl", KEY_HANDLER_LEFT);
  bind_key ("kr", KEY_HANDLER_RIGHT);
  bind_key ("kP", KEY_HANDLER_PAGE_UP);
  bind_key ("kN", KEY_HANDLER_PAGE_DOWN);

  // add mainloop source for keys
  static GSourceFuncs source_funcs = { stdin_prepare, stdin_check, stdin_dispatch, };
  GSource *source = g_source_new (&source_funcs, sizeof (GSource));
  g_source_attach (source, g_main_loop_get_context (loop));
  g_main_context_add_poll (g_main_loop_get_context (loop), &stdin_poll_fd, G_PRIORITY_DEFAULT);

  signal (SIGCONT, signal_sig_cont);
}

void
Terminal::end()
{
  reset_terminal();
}

void
Terminal::read_stdin()
{
  char buffer[1024];
  int r = read (0, buffer, 1024);
  for (int bpos = 0; bpos < r; bpos++)
    {
      int c = (unsigned char) buffer[bpos];
      chars.push_back (c);
    }
}

int
Terminal::getchar()
{
  for (;;)
    {
      if (chars.empty())
        return -1;

      // interpret escape sequences at the start of the chars buffer
      for (map<string, int>::iterator ki = keys.begin(); ki != keys.end(); ki++)
        {
          const string& esc_seq = ki->first;

          if (esc_seq.size() <= chars.size())
            {
              bool match = true;
              for (size_t i = 0; i < esc_seq.size(); i++)
                {
                  if (esc_seq[i] != chars[i])
                    match = false;
                }
              if (match)
                {
                  chars.erase (chars.begin(), chars.begin() + esc_seq.size());
                  chars.insert (chars.begin(), ki->second);
                }
            }
        }
      // return one interpreted char
      int rc = *chars.begin();
      chars.erase (chars.begin());
      return rc;
    }
}
