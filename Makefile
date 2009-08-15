GSTREAMER_VERSION = gstreamer-0.10

CFLAGS = $(shell pkg-config --cflags $(GSTREAMER_VERSION)) -Wall -O2
CXXFLAGS = $(shell pkg-config --cflags $(GSTREAMER_VERSION)) -Wall -O2
LDFLAGS = $(shell pkg-config --libs $(GSTREAMER_VERSION))

OBJS = glib-extra.o gst123.o

all: gst123 gst123.1

gst123: $(OBJS)
	$(CXX) -o gst123 $(OBJS) $(LDFLAGS)

gst123.1: gst123.1.doxi
	~/src/beast/doxer/doxer.py doxi2man gst123.1.doxi

clean:
	rm -f $(OBJS) gst123
