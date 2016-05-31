AM_CXXFLAGS = $(GSTREAMER_CFLAGS) $(GSTREAMER_GTK_CFLAGS) $(NCURSES_CFLAGS)
AM_CFLAGS = $(GSTREAMER_CFLAGS) $(GSTREAMER_GTK_CFLAGS) $(NCURSES_CFLAGS)

bin_PROGRAMS = gst123

gst123_SOURCES = gst123.cc glib-extra.c glib-extra.h terminal.cc terminal.h gtkinterface.h gtkinterface.cc keyhandler.h \
                 options.cc options.h microconf.cc microconf.h configfile.cc configfile.h \
		 uri.cc playlist.cc m3uparser.cc plsparser.cc uri.h playlist.h m3uparser.h plsparser.h \
		 iostream.cc iostream.h networkstream.cc filestream.cc consolestream.cc \
		 httpstream.cc visualization.h visualization.cc msg.h msg.cc typefinder.h typefinder.cc \
                 compat.h compat.cc utils.h utils.cc
gst123_LDADD = $(GSTREAMER_LIBS) $(GSTREAMER_GTK_LIBS) $(NCURSES_LIBS)
