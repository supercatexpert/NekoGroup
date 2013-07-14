#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_CONFIG_MACRO_DIR(m4)
m4_define([ng_major_version], [0])
m4_define([ng_minor_version], [1])
m4_define([ng_micro_version], [0])
m4_define([ng_version], [ng_major_version.ng_minor_version.ng_micro_version])

AC_PREREQ([2.61])
AC_INIT([NekoGroup], [ng_version], [https://github.com/supercatexpert/NekoGroup/issues/])
AM_INIT_AUTOMAKE([NekoGroup], [ng_version])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

AM_SILENT_RULES([yes])

NG_MAJOR_VERSION=ng_major_version
NG_MINOR_VERSION=ng_minor_version
NG_MICRO_VERSIOn=ng_micro_version
NG_VERSION=ng_version

AC_SUBST(NG_MAJOR_VERSION)
AC_SUBST(NG_MINOR_VERSION)
AC_SUBST(NG_MICRO_VERSION)
AC_SUBST(NG_VERSION)

AC_DEFINE(NG_MAJOR_VERSION, [ng_major_version],
    [Define to the NekoGroup major version])
AC_DEFINE(NG_MINOR_VERSION, [ng_minor_version],
    [Define to the NekoGroup minor version])
AC_DEFINE(NG_MICRO_VERSION, [ng_micro_version],
    [Define to the NekoGroup micro version])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
CFLAGS="-Wall -O2 -DINSTALL_PATH=\\\"$datadir/NekoGroup/\\\""

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
if test "$GLIB_GENMARSHAL" = no; then
    AC_MSG_ERROR([glib-genmarshal not found!])
fi

# Check for gtk-doc
m4_ifdef([GTK_DOC_CHECK], [GTK_DOC_CHECK([1.14],[--flavour no-tmpl])], \
    [AM_CONDITIONAL([ENABLE_GTK_DOC], false)])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB2], [glib-2.0 >= 2.32, gthread-2.0 >= 2.32, \
    gmodule-2.0 >= 2.32, gio-2.0 >= 2.32])
PKG_CHECK_MODULES([LOUDMOUTH], [loudmouth-1.0])
PKG_CHECK_MODULES([MONGOCLIENT], [libmongo-client])

# Check additional libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stdio.h limits.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([bzero memset realpath strrchr strstr])

# Add the languages which your application supports to po/LINGUAS
IT_PROG_INTLTOOL([0.40.0])
GETTEXT_PACKAGE=NekoGroup
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", \
    [Define to the Gettext package name])
AM_GLIB_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

GOBJECT_INTROSPECTION_CHECK([1.32.0])

# Checks for features

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], \
    [enable debug mode by default]), enable_debug=yes, \
    enable_debug=no)
AM_CONDITIONAL(DEBUG_MODE, test "x$enable_debug" = "xyes")

AC_MSG_CHECKING([for Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in po/Makefile
])
AC_OUTPUT

echo "
$PACKAGE $VERSION

Debug Mode Enabled...........: $enable_debug

configure complete, now type 'make'
"



