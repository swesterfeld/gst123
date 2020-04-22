include $(top_srcdir)/Makefile.decl

AUTOMAKE_OPTIONS = 1.9 dist-bzip2 no-dist-gzip
ACLOCAL_AMFLAGS = -Wno-portability

EXTRA_DIST += TODO
SUBDIRS = src docs

desktopdir = $(datadir)/applications
dist_desktop_DATA = gst123.desktop

# === Release auxillary rule configuration ===
RELEASE_DESTHOST  = space.twc.de
RELEASE_DESTPATH  = public_html/gst123
RELEASE_CHANGELOG = ChangeLog
# Makefile.decl introduced rules: ChangeLog release-upload
