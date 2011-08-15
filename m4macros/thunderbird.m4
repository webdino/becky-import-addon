AC_DEFUN([AC_CHECK_THUNDERBIRD_BUILD_ENVIRONMENT],
[
  AC_ARG_WITH(thunderbird-built-dir,
  [  --with-thunderbird-built-dir=PFX   Set the built directory of Thunderbird at <PFX>],
    THUNDERBIRD_BUILT_DIR=$withval)
  
  if test -n "$THUNDERBIRD_BUILT_DIR" -a "$THUNDERBIRD_BUILT_DIR" != "no"; then
      THUNDERBIRD_BUILT_DIR=`cd "$THUNDERBIRD_BUILT_DIR" && pwd`
      XPCOM_LDFLAGS="-L$THUNDERBIRD_BUILT_DIR/mozilla/dist/lib"
      XPCOM_CFLAGS="-fshort-wchar"
      XPCOM_CFLAGS="$XPCOM_CFLAGS -I$THUNDERBIRD_BUILT_DIR/mozilla/dist/include/mozilla"
      XPCOM_CFLAGS="$XPCOM_CFLAGS -I$THUNDERBIRD_BUILT_DIR/mozilla/dist/include"
  else
      AC_MSG_ERROR([--with-thunderbird-built-dir must specify a path])
  fi
  XPCOM_LIBS="-lxpcomglue_s -lnspr4 -lplds4"

  MJ_CHECK_NSIIMPORTADDRESSBOOKS_INTERFACE
  AC_SUBST(THUNDERBIRD_BUILT_DIR)
  AC_SUBST(XPCOM_LIBS)
  AC_SUBST(XPCOM_LDFLAGS)
  AC_SUBST(XPCOM_CFLAGS)
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
        [AC_DEFINE([MOZ_IMPORTADDRESSBOOK_NEED_ISADDRLOCHOME],[1],[Define if ImportAdressBook method needs isAddrLogHome]) no_importaddressbook_need_isaddrlochome=yes],
        [no_importaddressbook_need_isaddrlochome=no])
  AC_MSG_RESULT([$no_importaddressbook_need_isaddrlochome])
  CPPFLAGS=$_SAVE_CPPFLAGS
  CXXFLAGS=$_SAVE_CXXFLAGS

  AC_LANG_POP([C++])
])
