AC_PREREQ(2.59)
AC_INIT(src/control.c)
AM_INIT_AUTOMAKE(diet-alsa-lib, 0.0.1)
AC_PREFIX_DEFAULT(/usr)

dnl AC_CONFIG_HEADERS(src/config.h)

AC_PROG_CC
AC_PROG_INSTALL
dnl AC_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_HEADER_STDC

AC_ARG_ENABLE(mixer,
  AS_HELP_STRING([--disable-mixer],
		 [disable mixer interface]),
  mixer="$enableval", mixer="yes")
AC_ARG_ENABLE(rawmidi,
  AS_HELP_STRING([--disable-rawmidi],
		 [disable rawmidi interface]),
  rawmidi="$enableval", rawmidi="yes")
AC_ARG_ENABLE(hwdep,
  AS_HELP_STRING([--disable-hwdep],
		 [disable hwdep interface]),
  hwdep="$enableval", hwdep="yes")

AM_CONDITIONAL(BUILD_MIXER, test "$mixer" = "yes")
AM_CONDITIONAL(BUILD_RAWMIDI, test "$rawmidi" = "yes")
AM_CONDITIONAL(BUILD_HWDEP, test "$hwdep" = "yes")

AC_OUTPUT([
	Makefile
	alsa.pc
	src/Makefile
])
