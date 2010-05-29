AM_CXXFLAGS = $(GSTREAMER_CFLAGS) $(GSTREAMER_GTK_CFLAGS)
AM_CFLAGS = $(GSTREAMER_CFLAGS) $(GSTREAMER_GTK_CFLAGS)

bin_PROGRAMS = gst123

gst123_SOURCES = gst123.cc glib-extra.c glib-extra.h terminal.cc terminal.h gtkinterface.h gtkinterface.cc keyhandler.h \
                 options.cc options.h
gst123_LDADD = $(GSTREAMER_LIBS) $(GSTREAMER_GTK_LIBS)
