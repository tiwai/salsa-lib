AC_PREREQ(2.59)
AC_INIT(src/control.c)
AM_INIT_AUTOMAKE(salsa-lib, 0.0.1)
AC_PREFIX_DEFAULT(/usr)

dnl AC_CONFIG_HEADERS(src/config.h)

AC_PROG_CC
AC_PROG_INSTALL
dnl AC_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_HEADER_STDC

dnl Component selections

AC_ARG_ENABLE(mixer,
  AS_HELP_STRING([--disable-mixer],
		 [disable mixer interface]),
  mixer="$enableval", mixer="yes")
AC_ARG_ENABLE(rawmidi,
  AS_HELP_STRING([--enable-rawmidi],
		 [enable rawmidi interface]),
  rawmidi="$enableval", rawmidi="no")
AC_ARG_ENABLE(hwdep,
  AS_HELP_STRING([--enable-hwdep],
		 [enable hwdep interface]),
  hwdep="$enableval", hwdep="no")
AC_ARG_ENABLE(timer,
  AS_HELP_STRING([--enable-timer],
		 [enable timer interface]),
  timer="$enableval", timer="no")
AC_ARG_ENABLE(conf,
  AS_HELP_STRING([--enable-conf],
		 [enable dummy conf functions]),
  sndconf="$enableval", sndconf="no")
AC_ARG_ENABLE(seq,
  AS_HELP_STRING([--enable-seq],
		 [enable seq functions]),
  sndseq="$enableval", sndseq="no")

AC_ARG_ENABLE(tlv,
  AS_HELP_STRING([--enable-tlv],
	 	 [enable TLV (dB) support]),
  tlv="$enableval", tlv="no")

AC_ARG_ENABLE(seq,
  AS_HELP_STRING([--enable-everyhing],
		 [enable everyhing :)]),
  everything="$enableval", everything="no")

if test "$everything" = "yes"; then
  mixer=yes
  rawmidi=yes
  hwdep=yes
  timer=yes
  sndconf=yes
  sndseq=yes
  tlv=yes
fi

AM_CONDITIONAL(BUILD_MIXER, test "$mixer" = "yes")
AM_CONDITIONAL(BUILD_RAWMIDI, test "$rawmidi" = "yes")
AM_CONDITIONAL(BUILD_HWDEP, test "$hwdep" = "yes")
AM_CONDITIONAL(BUILD_TIMER, test "$timer" = "yes")
AM_CONDITIONAL(BUILD_CONF, test "$sndconf" = "yes")
AM_CONDITIONAL(BUILD_SEQ, test "$sndseq" = "yes")

if test "$tlv" = "yes"; then
  SALSA_HAS_TLV_SUPPORT=1
else
  SALSA_HAS_TLV_SUPPORT=0
fi
AC_SUBST(SALSA_HAS_TLV_SUPPORT)

dnl OK, let's output...

AC_OUTPUT([
	Makefile
	alsa.pc
	src/Makefile
	src/recipe.h
])

echo "Creating asoundlib.h..."
cp "$srcdir"/src/asoundlib-head.h src/asoundlib.h
test "$mixer" = "yes" && echo "#include \"mixer.h\"" >> src/asoundlib.h
test "$rawmidi" = "yes" && echo "#include \"rawmidi.h\"" >> src/asoundlib.h
test "$hwdep" = "yes" && echo "#include \"hwdep.h\"" >> src/asoundlib.h
test "$timer" = "yes" && echo "#include \"timer.h\"" >> src/asoundlib.h
test "$sndconf" = "yes" && echo "#include \"conf.h\"" >> src/asoundlib.h
test "$sndseq" = "yes" && echo "#include \"seq_event.h\"" >> src/asoundlib.h
test "$sndseq" = "yes" && echo "#include \"seq.h\"" >> src/asoundlib.h
test "$sndseq" = "yes" && echo "#include \"seqmid.h\"" >> src/asoundlib.h
cat "$srcdir"/src/asoundlib-tail.h >> src/asoundlib.h

