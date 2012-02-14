AC_INIT([gst123], [0.2.2])
AC_CONFIG_SRCDIR([src/gst123.cc])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE

dnl ----------------------------- from acbirnet.m4 -------------------------------
dnl Check whether cc accepts a certain option
dnl MC_PROG_CC_SUPPORTS_OPTION(OPTIONS, ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND])
AC_DEFUN([MC_PROG_CC_SUPPORTS_OPTION], [
AC_MSG_CHECKING([whether ${CC-cc} supports $1])
echo >conftest.c;
if ${CC-cc} [$1] -c $CFLAGS conftest.c >/dev/null 2>&1 ; then
    AC_MSG_RESULT(yes)
    [$2]
else
    AC_MSG_RESULT(no)
    [$3]
fi
rm -fr conftest*
])dnl

# debug builds
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debugging (default=no)]), ENABLE_DEBUG=$enableval)
DEBUG_FLAGS=""
if test "$ENABLE_DEBUG" = "yes"; then
  MC_PROG_CC_SUPPORTS_OPTION([-g], [ DEBUG_FLAGS="-g" ])
fi

if test -z "$CFLAGS"; then
  CFLAGS="$DEBUG_FLAGS"
else
  CFLAGS="$CFLAGS $DEBUG_FLAGS"
fi
if test -z "$CXXFLAGS"; then
  CXXFLAGS="$DEBUG_FLAGS"
else
  CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS"
fi

# optimizer settings
MC_PROG_CC_SUPPORTS_OPTION([-O2], [
  CFLAGS="$CFLAGS -O2"
  CXXFLAGS="$CXXFLAGS -O2"
])

AC_PROG_CXX
AC_PROG_CC
# disable automatic maintainer mode
AM_MAINTAINER_MODE

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
dnl Define gstreamer interface
dnl
AC_DEFUN([AC_GSTREAMER_GTK_REQUIREMENTS],
[
    dnl We need GLib with friends
    PKG_CHECK_MODULES(GSTREAMER_GTK, gstreamer-base-0.10 gtk+-2.0 gstreamer-interfaces-0.10 gstreamer-video-0.10)
    AC_SUBST(GSTREAMER_GTK_CFLAGS)
    AC_SUBST(GSTREAMER_GTKLIBS)
])

dnl
dnl We need libX11
dnl
AC_DEFUN([AC_X11_REQUIREMENTS],
[
    AC_CHECK_LIB(X11, XOpenDisplay)
    X11_LIBS='-lX11'
    AC_SUBST(X11_LIBS)
])

dnl
dnl We need libncurses
dnl
AC_DEFUN([AC_NCURSES_REQUIREMENTS],
[
    AC_PATH_PROG(NCURSES5_CONFIG,ncurses5-config,no)
    if test "$NCURSES5_CONFIG" = "no"; then
      AC_MSG_ERROR([You need to have ncurses5-config installed to build this package.

Debian users: aptitude install libncurses-dev
])
    fi
    AC_CHECK_LIB(ncurses, initscr,[:],[
    AC_MSG_ERROR([You need libncurses to build this package.

Debian users: aptitude install libncurses-dev
])
])
    NCURSES_CFLAGS="`$NCURSES5_CONFIG --cflags`"
    NCURSES_LIBS="`$NCURSES5_CONFIG --libs`"
    AC_SUBST(NCURSES_CFLAGS)
    AC_SUBST(NCURSES_LIBS)
])

dnl
dnl Manpage from wiki (required for --enable-maintainer-mode)
dnl
AC_DEFUN([AC_MAN_REQUIREMENTS],
[
  AC_PATH_PROGS(WIKIHTML2MAN, wikihtml2man wikihtml2man.py, false)
  AC_PATH_PROG(GROFF, groff, false)
  IFMAN=
  IFAGING=
  if test x"$enable_maintainer_mode" = xyes ; then
    test x"$WIKIHTML2MAN" = xfalse && {
      AC_MSG_ERROR(This package requires the wikihtml2man.py tool for maintainer builds)
    }
    echo "" | $GROFF -mandoc -Thtml 2>/dev/null | grep -Fq HTML || {
      AC_MSG_ERROR(Failed to test-build HTML manual page via groff)
    }
  else
    IFMAN="#"
    IFAGING="#"
  fi
  AC_SUBST(IFMAN)
  AC_SUBST(IFAGING)
])


# Less cluttered build output
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Check requirement sets
AC_GSTREAMER_REQUIREMENTS
AC_GSTREAMER_GTK_REQUIREMENTS
AC_X11_REQUIREMENTS
AC_NCURSES_REQUIREMENTS
AC_MAN_REQUIREMENTS

MC_PROG_CC_SUPPORTS_OPTION([-Wall], [
  CFLAGS="$CFLAGS -Wall"
  CXXFLAGS="$CXXFLAGS -Wall"
])

AC_ARG_ENABLE(debug-cxx,AS_HELP_STRING([--enable-debug-cxx], [setup compiler flags to do C++ STL debug checks]),
[
  CFLAGS="$CFLAGS -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC"
  CXXFLAGS="$CXXFLAGS -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC"
])

AC_CONFIG_FILES([Makefile src/Makefile docs/Makefile])
AC_OUTPUT
