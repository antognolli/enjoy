AC_INIT(enjoy, 0.1.0.1, contact@profusion.mobi)
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(configure.ac)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_ISC_POSIX

AM_INIT_AUTOMAKE(1.6 dist-bzip2)
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])
AM_CONFIG_HEADER(config.h)
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")
_XTERM_COLORS

AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_HEADER_STDC
dnl AC_C_CONST is bogus, its tests fails on -Wall -O1 ( uninitialized values)
dnl AC_C_CONST

define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
AC_PROG_LIBTOOL

VMAJ=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $1);}'`
VMIN=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $2);}'`
VMIC=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $3);}'`
SNAP=`echo $PACKAGE_VERSION | awk -F. '{printf("%s", $4);}'`
version_info=`expr $VMAJ + $VMIN`":$VMIC:$VMIN"
AC_SUBST(version_info)

themesdir="${datadir}/enjoy/data/themes"
AC_SUBST(themesdir)
AS_AC_EXPAND(THEMESDIR, $themesdir)
AC_DEFINE_UNQUOTED(THEMESDIR, ["$THEMESDIR"], [Where theme files are installed.])

AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
AC_DEFINE_UNQUOTED(SYSCONFDIR, ["$SYSCONFDIR"], [Where system configuration is stored])

PKG_CHECK_MODULES(EET, [eet])
PKG_CHECK_MODULES(EVAS, [evas])
PKG_CHECK_MODULES(ECORE, [ecore])
PKG_CHECK_MODULES(ECORE_EVAS, [ecore-evas])
PKG_CHECK_MODULES(EDJE, [edje])
PKG_CHECK_MODULES(EMOTION, [emotion])

PKG_CHECK_MODULES(GUARANA, [guarana])
PKG_CHECK_MODULES(GUARANA_WIDGETS, [guarana_widgets])
PKG_CHECK_MODULES(GUARANA_UTILS, [guarana_utils])

PKG_CHECK_MODULES(LMS, [lightmediascanner])

PKG_CHECK_MODULES(SQLITE3, [sqlite3 >= 3.3])

AC_OUTPUT([
Makefile
src/Makefile
src/bin/Makefile
src/lib/Makefile
src/tests/Makefile
m4/Makefile
data/Makefile
data/enjoy.desktop
data/themes/Makefile
data/themes/enjoy/Makefile
data/themes/enjoy/fonts/Makefile
data/themes/enjoy/images/Makefile
config/Makefile
])
