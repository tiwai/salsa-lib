AM_CFLAGS = -Wall -g

lib_LTLIBRARIES = libasound.la
libasound_la_SOURCES = control.c hcontrol.c mixer.c cards.c \
	pcm.c pcm_params.c pcm_misc.c \
	rawmidi.c \
	hwdep.c
libasound_la_LDFLAGS = -version-number 2:0:0

alsaincludedir = $(includedir)/alsa

alsainclude_HEADERS = asoundlib.h version.h asound.h asoundef.h \
	global.h output.h error.h \
	control.h ctl_types.h ctl_macros.h \
	hcontrol.h hctl_types.h hctl_macros.h \
	mixer.h mixer_types.h mixer_macros.h \
	pcm.h pcm_types.h pcm_macros.h \
	rawmidi.h rawmidi_types.h rawmidi_macros.h \
	hwdep.h hwdep_types.h hwdep_macros.h

noinst_HEADERS = local.h
