AC_PREREQ(2.59)
AC_INIT(src/control.c)
AM_INIT_AUTOMAKE(salsa-lib, 0.1.6)
AC_PREFIX_DEFAULT(/usr)

dnl AC_CONFIG_HEADERS(src/config.h)

AC_PROG_CC
AC_PROG_INSTALL
dnl AC_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_HEADER_STDC

dnl Compatible version
AC_ARG_WITH(compat-version,
  AS_HELP_STRING([--with-compat-version=VERSION],
		 [specify the compatible version with ALSA-lib (default=1.0.28)]),
  compatver="$withval", compatver="1.0.28")

SND_LIB_VERSION="$compatver"
SND_LIB_MAJOR=1
SND_LIB_MINOR=0
SND_LIB_SUBMINOR=28
SND_LIB_EXTRAVER=1000000
case "$compatver" in
*.*.*)
  SND_LIB_MAJOR=`echo $SND_LIB_VERSION | cut -d . -f 1`
  SND_LIB_MINOR=`echo $SND_LIB_VERSION | cut -d . -f 2`
  SND_LIB_SUBMINOR=`echo $SND_LIB_VERSION | cut -d . -f 3`
  ;;
esac
AC_SUBST(SND_LIB_VERSION)
AC_SUBST(SND_LIB_MAJOR)
AC_SUBST(SND_LIB_MINOR)
AC_SUBST(SND_LIB_SUBMINOR)
AC_SUBST(SND_LIB_EXTRAVER)

dnl Non-standard device files directory
AC_MSG_CHECKING([for ALSA device file directory])
AC_ARG_WITH(alsa-devdir,
  AS_HELP_STRING([--with-alsa-devdir=dir],
		 [directory with ALSA device files (default /dev/snd)]),
  [alsa_dev_dir="$withval"],
  [alsa_dev_dir="/dev/snd"])
DEVPATH="$alsa_dev_dir"
AC_SUBST(DEVPATH)
AC_MSG_RESULT([$alsa_dev_dir])

dnl Component selections

AC_ARG_ENABLE(pcm,
  AS_HELP_STRING([--disable-pcm],
		 [disable PCM interface]),
  pcm="$enableval", pcm="yes")
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

AC_ARG_ENABLE(user-elem,
  AS_HELP_STRING([--disable-user-elem],
	 	 [disable user-space control element support]),
  user_elem="$enableval", user_elem="yes")

AC_ARG_ENABLE(async,
  AS_HELP_STRING([--enable-async],
		 [enable async handler support]),
  async="$enableval", async="no")

AC_ARG_ENABLE(chmap,
  AS_HELP_STRING([--enable-chmap],
		 [enable chmap API support]),
  chmap="$enableval", chmap="no")

AC_ARG_ENABLE(abi-compat,
  AS_HELP_STRING([--enable-abi-compat],
		 [build ABI-compatible library with alsa-lib]),
  abi_compat="$enableval", abi_compat="no")

AC_ARG_ENABLE(deprecated,
  AS_HELP_STRING([--disable-deprecated],
		 [don't mark deprecated attribute for non-working functions]),
  markdeprecated="$enableval", markdeprecated="yes")

AC_ARG_ENABLE(output-buffer,
  AS_HELP_STRING([--enable-output-buffer],
		 [support the string output via snd_output_*() functions]),
  output_buffer="$enableval", output_buffer="no")

AC_ARG_ENABLE(delight-valgrind,
  AS_HELP_STRING([--disable-delight-valgrind],
		 [do not initialize unnecessary fields for ioctls]),
  delight_valgrind="$enableval", delight_valgrind="yes")

AC_ARG_ENABLE(symbolic-functions,
  AS_HELP_STRING([--disable-symbolic-functions],
    [use -Bsymbolic-functions option if available (optmization for size and speed)]),
  symfuncs="$enableval", symfuncs="yes")

AC_ARG_ENABLE(float,
  AS_HELP_STRING([--enable-float],
		 [support floatin-point unit]),
  support_float="$enableval", support_float="no")

AC_ARG_ENABLE(4bit,
  AS_HELP_STRING([--disable-4bit],
		 [drop the support for 4bit PCM (IMA ADPCM)]),
  support_4bit="$enableval", support_4bit="yes")

AC_ARG_ENABLE(abicheck,
  AS_HELP_STRING([--enable-abicheck],
		 [enable library ABI check]),
  abicheck="$enableval", abicheck="no")

AC_ARG_ENABLE(ctlasciiparser,
  AS_HELP_STRING([--enable-ctlasciiparser],
		 [enable ctl ASCII parser (for amixer)]),
  ctlasciiparser="$enableval", ctlasciiparser="no")

AC_ARG_ENABLE(time64,
  AS_HELP_STRING([--enable-time64],
		 [enforce 64bit timespec for ioctls]),
  struct_time64="$enableval", struct_time64="no")

AC_ARG_ENABLE(everything,
  AS_HELP_STRING([--enable-everything],
		 [enable everything :)]),
  everything="$enableval", everything="no")

if test "$everything" = "yes"; then
  pcm="yes"
  mixer="yes"
  rawmidi="yes"
  hwdep="yes"
  timer="yes"
  sndconf="yes"
  sndseq="yes"
  tlv="yes"
  user_elem="yes"
  async="yes"
  chmap="yes"
  abi_compat="yes"
  symfuncs="yes"
  output_buffer="yes"
  support_float="yes"
  support_4bit="yes"
  abicheck="yes"
  ctlasciiparser="yes"
  struct_time64="yes"
fi

SALSA_DEPLIBS=""

AM_CONDITIONAL(BUILD_PCM, test "$pcm" = "yes")
AM_CONDITIONAL(BUILD_MIXER, test "$mixer" = "yes")
AM_CONDITIONAL(BUILD_RAWMIDI, test "$rawmidi" = "yes")
AM_CONDITIONAL(BUILD_HWDEP, test "$hwdep" = "yes")
AM_CONDITIONAL(BUILD_TIMER, test "$timer" = "yes")
AM_CONDITIONAL(BUILD_CONF, test "$sndconf" = "yes")
AM_CONDITIONAL(BUILD_SEQ, test "$sndseq" = "yes")
AM_CONDITIONAL(BUILD_ASYNC, test "$async" = "yes")

if test "$tlv" = "yes"; then
  SALSA_HAS_TLV_SUPPORT=1
else
  SALSA_HAS_TLV_SUPPORT=0
fi
AC_SUBST(SALSA_HAS_TLV_SUPPORT)

if test "$user_elem" = "yes"; then
  SALSA_HAS_USER_ELEM_SUPPORT=1
else
  SALSA_HAS_USER_ELEM_SUPPORT=0
fi
AC_SUBST(SALSA_HAS_USER_ELEM_SUPPORT)

if test "$async" = "yes"; then
  SALSA_HAS_ASYNC_SUPPORT=1
else
  SALSA_HAS_ASYNC_SUPPORT=0
fi
AC_SUBST(SALSA_HAS_ASYNC_SUPPORT)

if test "$chmap" = "yes"; then
  SALSA_HAS_CHMAP_SUPPORT=1
else
  SALSA_HAS_CHMAP_SUPPORT=0
fi
AC_SUBST(SALSA_HAS_CHMAP_SUPPORT)

if test "$sndconf" = "yes"; then
  SALSA_HAS_DUMMY_CONF=1
else
  SALSA_HAS_DUMMY_CONF=0
fi
AC_SUBST(SALSA_HAS_DUMMY_CONF)

AM_CONDITIONAL(COMPAT_ABI, test "$abi_compat" = "yes")

if test "$markdeprecated" = "yes"; then
  SALSA_MARK_DEPRECATED=1
else
  SALSA_MARK_DEPRECATED=0
fi
AC_SUBST(SALSA_MARK_DEPRECATED)

if test "$output_buffer" = "yes"; then
  SALSA_SUPPORT_OUTPUT_BUFFER=1
else
  SALSA_SUPPORT_OUTPUT_BUFFER=0
fi
AC_SUBST(SALSA_SUPPORT_OUTPUT_BUFFER)

if test "$support_float" = "yes"; then
  SALSA_SUPPORT_FLOAT=1
  SALSA_DEPLIBS="$SALSA_DEPLIBS -lm"
else
  SALSA_SUPPORT_FLOAT=0
fi
AC_SUBST(SALSA_SUPPORT_FLOAT)

if test "$support_4bit" = "yes"; then
  SALSA_SUPPORT_4BIT_PCM=1
else
  SALSA_SUPPORT_4BIT_PCM=0
fi
AC_SUBST(SALSA_SUPPORT_4BIT_PCM)

if test "$abicheck" = "yes"; then
  SALSA_CHECK_ABI=1
else
  SALSA_CHECK_ABI=0
fi
AC_SUBST(SALSA_CHECK_ABI)

if test "$ctlasciiparser" = "yes"; then
  SALSA_CTL_ASCII_PARSER=1
else
  SALSA_CTL_ASCII_PARSER=0
fi
AC_SUBST(SALSA_CTL_ASCII_PARSER)

if test "$struct_time64" = "yes"; then
  SALSA_STRUCT_TIME64=1
else
  SALSA_STRUCT_TIME64=0
fi
AC_SUBST(SALSA_STRUCT_TIME64)

if test "$delight_valgrind" = "yes"; then
  AC_DEFINE(DELIGHT_VALGRIND)
fi

AC_MSG_CHECKING(for symbolic-functions)
if test "$symfuncs" = "yes"; then
  if ld --help | grep -q -- '-Bsymbolic-functions'; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(not supported by ld)
    symfuncs="no"
  fi
else
  AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(SYMBOLIC_FUNCTIONS, test x"$symfuncs" = xyes)

AC_SUBST(SALSA_DEPLIBS)

dnl OK, let's output...

AC_OUTPUT([
	Makefile
	alsa.pc
	src/Makefile
	src/recipe.h
	src/version.h
])

echo "Creating asoundlib.h..."
cp "$srcdir"/src/asoundlib-head.h src/asoundlib.h
test "$pcm" = "yes" && echo "#include \"pcm.h\"" >> src/asoundlib.h
test "$mixer" = "yes" && echo "#include \"mixer.h\"" >> src/asoundlib.h
test "$rawmidi" = "yes" && echo "#include \"rawmidi.h\"" >> src/asoundlib.h
test "$hwdep" = "yes" && echo "#include \"hwdep.h\"" >> src/asoundlib.h
test "$timer" = "yes" && echo "#include \"timer.h\"" >> src/asoundlib.h
test "$sndconf" = "yes" && echo "#include \"conf.h\"" >> src/asoundlib.h
test "$sndseq" = "yes" && echo "#include \"seq_event.h\"" >> src/asoundlib.h
test "$sndseq" = "yes" && echo "#include \"seq.h\"" >> src/asoundlib.h
test "$sndseq" = "yes" && echo "#include \"seqmid.h\"" >> src/asoundlib.h
cat "$srcdir"/src/asoundlib-tail.h >> src/asoundlib.h

echo "SALSA-Lib build configuration:"
echo "  - PCM interface: $pcm"
echo "  - Mixer interface: $mixer"
echo "  - Raw MIDI interface: $rawmidi"
echo "  - HW-dependent interface: $hwdep"
echo "  - Timer interface: $timer"
echo "  - ALSA-config dummy interface: $sndconf"
echo "  - ALSA-sequencer dummy interface: $sndseq"
echo "  - TLV (dB) support: $tlv"
echo "  - User-space control element support: $user_elem"
echo "  - Async handler support: $async"
echo "  - PCM chmap API support: $chmap"
echo "  - Make ABI-compatible libasound.so: $abi_compat"
echo "  - Mark deprecated attribute: $markdeprecated"
echo "  - Support string-output via snd_output: $output_buffer"
echo "  - Support floating-point: $support_float"
echo "  - Enable library ABI check: $abicheck"
echo "  - Support ctl ASCII parser: $ctlasciiparser"
echo "  - Enforce 64bit timespec: $struct_time64"
