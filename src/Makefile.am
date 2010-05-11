AM_CXXFLAGS = $(GSTREAMER_CFLAGS) $(GSTREAMER_GTK_CFLAGS)
AM_CFLAGS = $(GSTREAMER_CFLAGS) $(GSTREAMER_GTK_CFLAGS)

bin_PROGRAMS = gst123

gst123_SOURCES = gst123.cc glib-extra.c glib-extra.h terminal.cc terminal.h gtkinterface.h gtkinterface.cc
gst123_LDADD = $(GSTREAMER_LIBS) $(GSTREAMER_GTK_LIBS)
dist_man_MANS = gst123.1

rebuild-manpage:
	~/src/beast/doxer/doxer.py doxi2man gst123.1.doxi
