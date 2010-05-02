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

#include <vector>
#include <map>

#ifdef getch
#undef getch
#endif

#include "terminal.h"

using std::vector;
using std::string;
using std::map;

static char term_buffer[4096];
static char term_buffer2[4096];
static char *term_p = term_buffer2;

void
Terminal::init()
{
  const char *termtype = NULL;
  termtype = getenv ("TERM");
  if (termtype)
    terminal_type = termtype;
  else
    terminal_type = "unknown";

  tgetent (term_buffer, terminal_type.c_str());

  // enable keypad xmit
  printf ("%s", tgetstr ("ks", &term_p));

  tcgetattr (0, &tio_orig);

  struct termios tio_new = tio_orig;
  tio_new.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
  tio_new.c_cc[VMIN] = 0;
  tio_new.c_cc[VTIME] = 1; /* 0.1 seconds */
  tcsetattr (0, TCSANOW, &tio_new);

  // initialize common keyboard escape sequences
  keys[tgetstr ("ku", &term_p)] = TERMINAL_KEY_UP;
  keys[tgetstr ("kd", &term_p)] = TERMINAL_KEY_DOWN;
  keys[tgetstr ("kl", &term_p)] = TERMINAL_KEY_LEFT;
  keys[tgetstr ("kr", &term_p)] = TERMINAL_KEY_RIGHT;
  keys[tgetstr ("kP", &term_p)] = TERMINAL_KEY_PAGE_UP;
  keys[tgetstr ("kN", &term_p)] = TERMINAL_KEY_PAGE_DOWN;
}

void
Terminal::end()
{
  tcsetattr(0,TCSANOW,&tio_orig);

  // disable keypad xmit
  printf ("%s", tgetstr ("ke", &term_p));
}

int
Terminal::getch()
{
  for (;;)
    {
      char buffer[1024];
      int r = read (0, buffer, 1024);
      if (r >= 1)
        {
          for (int bpos = 0; bpos < r; bpos++)
            {
              int c = (unsigned char) buffer[bpos];
              chars.push_back (c);
            }
        }
      if (r == 0)
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
  return 0;
}
