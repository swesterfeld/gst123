AC_INIT([gst123], [0.0.3])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/gst123.cc])
AC_CONFIG_HEADER([config.h])
AC_PROG_CXX
AC_PROG_CC

dnl
dnl Define package requirements.
dnl
AC_DEFUN([AC_GSTREAMER_REQUIREMENTS],
[
    dnl We need GLib with friends
    PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10)
    AC_SUBST(GSTREAMER_CFLAGS)
    AC_SUBST(GSTREAMER_LIBS)
])

dnl
dnl We need libncurses
dnl
AC_DEFUN([AC_NCURSES_REQUIREMENTS],
[
    AC_CHECK_LIB(ncurses, initscr)
    NCURSES_LIBS='-lncurses'
    AC_SUBST(NCURSES_LIBS)
])

# Check requirement sets
AC_GSTREAMER_REQUIREMENTS
AC_NCURSES_REQUIREMENTS

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
