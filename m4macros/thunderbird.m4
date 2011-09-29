AC_DEFUN([AC_CHECK_THUNDERBIRD_BUILD_ENVIRONMENT],
[
  AC_ARG_WITH(thunderbird-built-dir,
  [  --with-thunderbird-built-dir=PFX   Set the built directory of Thunderbird at <PFX>],
    THUNDERBIRD_BUILT_DIR=$withval)
  
  if test -n "$THUNDERBIRD_BUILT_DIR" -a "$THUNDERBIRD_BUILT_DIR" != "no"; then
      THUNDERBIRD_BUILT_DIR=`cd "$THUNDERBIRD_BUILT_DIR" && pwd`
  else
      AC_MSG_ERROR([--with-thunderbird-built-dir must specify a path])
  fi

  XPCOM_CFLAGS="-fshort-wchar -DXPCOM_GLUE_USE_NSPR"
  XPCOM_CFLAGS="$XPCOM_CFLAGS -I$THUNDERBIRD_BUILT_DIR/mozilla/dist/include/mozilla"
  XPCOM_CFLAGS="$XPCOM_CFLAGS -I$THUNDERBIRD_BUILT_DIR/mozilla/dist/include"
  XPCOM_LDFLAGS="-L$THUNDERBIRD_BUILT_DIR/mozilla/dist/lib"
  XPCOM_LIBS="-lxpcomglue_s"

  THUNDERBIRD_INCLUDE_PATH="$THUNDERBIRD_BUILT_DIR/mozilla/dist/include"
  THUNDERBIRD_LIB_PATH="$THUNDERBIRD_BUILT_DIR/mozilla/dist/lib"

  case "$host_os" in
    mingw*|cygwin*)
      THUNDERBIRD_INCLUDE_PATH=`cygpath -w $THUNDERBIRD_INCLUDE_PATH`
      THUNDERBIRD_LIB_PATH=`cygpath -w $THUNDERBIRD_LIB_PATH`
      ;;
    *)
  esac

  MJ_CHECK_NSIIMPORTADDRESSBOOKS_INTERFACE
  MJ_CHECK_NSIIMPORTGENERIC_INTERFACE
  AC_SUBST(XPCOM_LIBS)
  AC_SUBST(XPCOM_LDFLAGS)
  AC_SUBST(XPCOM_CFLAGS)

  AC_SUBST(THUNDERBIRD_INCLUDE_PATH)
  AC_SUBST(THUNDERBIRD_LIB_PATH)
])

AC_DEFUN([MJ_CHECK_NSIIMPORTADDRESSBOOKS_INTERFACE],
[
  AC_LANG_PUSH(C++)
  _SAVE_CPPFLAGS=$CPPFLAGS
  _SAVE_CXXFLAGS=$CXXFLAGS
  _SAVE_AM_CXXFLAGS=$AM_CXXFLAGS
  CXXFLAGS="$CXXFLAGS $AM_CXXFLAGS"
  CPPFLAGS="$XPCOM_CFLAGS"

  AC_MSG_CHECKING([nsIImportAddressBooks::ImportAddressBook method arguments])
  AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
                [[#include <nsIImportAddressBooks.h>]],
                [[nsIImportAddressBooks *p;
                p->ImportAddressBook(nsnull, nsnull, nsnull, nsnull, PR_TRUE, nsnull, nsnull, nsnull);]]
                )],
        [AC_DEFINE([MOZ_IMPORTADDRESSBOOK_NEED_ISADDRLOCHOME],[1],[Define if ImportAdressBook method needs isAddrLocHome]) importaddressbook_need_isaddrlochome=yes],
        [importaddressbook_need_isaddrlochome=no])
  AC_MSG_RESULT([$importaddressbook_need_isaddrlochome])
  CPPFLAGS=$_SAVE_CPPFLAGS
  CXXFLAGS=$_SAVE_CXXFLAGS

  if test "x$importaddressbook_need_isaddrlochome" = "xyes"; then
    THUNDERBIRD_MACROS="MOZ_IMPORTADDRESSBOOK_NEED_ISADDRLOCHOME"
    AC_SUBST(THUNDERBIRD_MACROS)
  fi

  AC_LANG_POP([C++])
])

AC_DEFUN([MJ_CHECK_NSIIMPORTGENERIC_INTERFACE],
[
  AC_LANG_PUSH(C++)
  _SAVE_CPPFLAGS=$CPPFLAGS
  _SAVE_CXXFLAGS=$CXXFLAGS
  _SAVE_AM_CXXFLAGS=$AM_CXXFLAGS
  CXXFLAGS="$CXXFLAGS $AM_CXXFLAGS"
  CPPFLAGS="$XPCOM_CFLAGS"

  AC_MSG_CHECKING([nsIImportGeneric::BeginImport method arguments])
  AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
                [[#include <nsIImportGeneric.h>]],
                [[nsIImportGeneric *p;
                p->BeginImport(nsnull, nsnull, PR_TRUE, nsnull);]]
                )],
        [AC_DEFINE([MOZ_IMPORTGENERIC_NEED_ISADDRLOCHOME],[1],[Define if BeginImport method needs isAddrLocHome]) beginimport_need_isaddrlochome=yes],
        [beginimport_need_isaddrlochome=no])
  AC_MSG_RESULT([$beginimport_need_isaddrlochome])
  CPPFLAGS=$_SAVE_CPPFLAGS
  CXXFLAGS=$_SAVE_CXXFLAGS

  if test "x$beginimport_need_isaddrlochome" = "xyes"; then
    THUNDERBIRD_MACROS="MOZ_BEGINIMPORT_NEED_ISADDRLOCHOME"
    AC_SUBST(THUNDERBIRD_MACROS)
  fi

  AC_LANG_POP([C++])
])
