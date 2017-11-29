gst123
======

gst123 is available under the [LGPL2](https://github.com/swesterfeld/gst123/blob/master/COPYING) license
and implemented in C++.

# DESCRIPTION

The program gst123 is designed to be a more flexible command line player
in the spirit of ogg123 and mpg123, based on gstreamer. It plays all file
formats gstreamer supports, so if you have a music collection which
contains different file formats, like flac, ogg and mp3, you can use gst123
to play all your music files.

All video file formats gstreamer can play are also available.

* For a full description, visit the project page:
	http://space.twc.de/~stefan/gst123.php

* To submit bug reports and feature requests, visit:
	https://github.com/swesterfeld/gst123/issues


# REQUIREMENTS

You need a C++ compiler, gstreamer-0.10 or gstreamer-1.0, gtk2.0 and ncurses to
build gst123.

# INSTALLATION

In short, gst123 needs to be built and installed with:

	./configure
	make
	make install
