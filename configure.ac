dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT(becky-import, 0.9.0, [hiikezoe@mozilla-japan.org])
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AC_CONFIG_MACRO_DIR([m4macros])

AM_CONFIG_HEADER([config.h])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AM_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CPP

case "$host_os" in
  mingw*|cygwin*)
    PLATFORM="WINNT"
    COMPILER="msvc"
    ;;
  linux-*)
    PLATFORM="Linux"
    COMPILER="gcc3"
    ;;
esac
AC_SUBST(PLATFORM)
AC_SUBST(COMPILER)

case "$host_cpu" in
  i*)
    CPU="x86"
    ;;
  x86_64)
    CPU="x86_64"
    ;;
esac
AC_SUBST(CPU)

AC_CHECK_THUNDERBIRD_BUILD_ENVIRONMENT

SHARED_LIBRARY_SUFFIX="$shrext_cmds"
AC_SUBST(SHARED_LIBRARY_SUFFIX)

AM_CONDITIONAL([PLATFORM_WINNT], [test "x$PLATFORM" = "xWINNT"])
AM_CONDITIONAL([PLATFORM_LINUX], [test "x$PLATFORM" = "xLinux"])

EXTENSION_CXXFLAGS="-DXPCOM_GLUE_USE_NSPR"
EXTENSION_CXXFLAGS="$EXTENSION_CXXFLAGS $XPCOM_CFLAGS"
AC_SUBST(EXTENSION_CXXFLAGS)

AC_CONFIG_FILES([
Makefile
install.rdf
application.ini
chrome.manifest
components/Makefile
])

AC_OUTPUT
