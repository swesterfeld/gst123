AC_INIT([gst123],[0.3.5])
AC_CONFIG_SRCDIR([src/gst123.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign])

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
dnl We need gstreamer-0.10 or gstreamer-1.0
dnl
AC_DEFUN([AC_GSTREAMER_REQUIREMENTS],
[
    dnl gstreamer-1.0 development package installed?
    PKG_CHECK_MODULES(GSTREAMER_1_0, gstreamer-1.0, GST_1_0=1, GST_1_0=0)

    if test x$GST_1_0 = x1; then
      AC_MSG_NOTICE([Building gst123 using gstreamer version 1.0])

      PKG_CHECK_MODULES(GSTREAMER, gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)
    else
      dnl gstreamer-0.10 development package installed?
      PKG_CHECK_MODULES(GSTREAMER_0_10, gstreamer-0.10, GST_0_10=1, GST_0_10=0)

      if test x$GST_0_10 = x1; then
        AC_MSG_NOTICE([Building gst123 using gstreamer version 0.10])
        PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10 gstreamer-base-0.10 gstreamer-interfaces-0.10 gstreamer-video-0.10)
      else
        AC_MSG_ERROR([Neither gstreamer-0.10 nor gstreamer-1.0 was found using pkg-config.])
      fi
    fi
    AC_SUBST(GSTREAMER_CFLAGS)
    AC_SUBST(GSTREAMER_LIBS)
])

dnl
dnl Define gstreamer interface
dnl
AC_DEFUN([AC_GSTREAMER_GTK_REQUIREMENTS],
[
    dnl We need GLib with friends
    PKG_CHECK_MODULES(GSTREAMER_GTK, gtk+-3.0)
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
    PKG_CHECK_MODULES(NCURSES, [ncurses >= 5], ncurses_has_pkgconfig=1, ncurses_has_pkgconfig=0)
    if test x$ncurses_has_pkgconfig = x0; then
      AC_PATH_PROG(NCURSES_CONFIG,ncurses6-config,false)
      if test "$NCURSES_CONFIG" = false; then
        AC_PATH_PROG(NCURSES_CONFIG,ncurses5-config,false)
      fi
      if test "$NCURSES_CONFIG" = false; then
        AC_PATH_PROG(NCURSES_CONFIG,ncurses-config,false)
      fi
      if test "$NCURSES_CONFIG" = false; then
        AC_MSG_ERROR([Neither pkgconfig(ncurses) nor ncurses-config was found.])
      fi
      NCURSES_CFLAGS="`$NCURSES_CONFIG --cflags`"
      NCURSES_LIBS="`$NCURSES_CONFIG --libs`"
    fi
    AC_SUBST(NCURSES_CFLAGS)
    AC_SUBST(NCURSES_LIBS)
])

# Less cluttered build output
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Check requirement sets
AC_GSTREAMER_REQUIREMENTS
AC_GSTREAMER_GTK_REQUIREMENTS
AC_X11_REQUIREMENTS
AC_NCURSES_REQUIREMENTS

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
