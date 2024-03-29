SALSA-Lib - Small ALSA Library
==============================

GENERAL
-------

SALSA-Lib is a small, light-weight, hot and spicy version of the ALSA
library, mainly for embedded systems with limited resources.
The library is designed to be source-level compatible with ALSA
library API for limited contents.  Most of function calls are inlined,
and accesses directly to the hardware via system calls.
Some components like ALSA sequencer aren't supported, and most of all,
the alsa-lib plugins and configurations are completely dropped.  Thus,
neither dmix nor format conversion is available with SALSA-lib.

The current features are found in the sections below.

### CONFIG

* All functions ``snd_config_*()`` are replaced with dummy ones since
  SALSA-Lib doesn't support the configuration files.
  The apps accessing config stuff won't work properly as expected.
* Disabled as default

### PCM

* Supports only the hw layer, i.e. no up/down-mixing, format
  conversion, rate conversion, etc, and no external plug-in
* Accepts the limited PCM name, ``hw``, ``default``, ``default:x``,
  and ``hw:x,y,z``
* Some of ``snd_pcm_mmap_read/write*()`` functions are declared but
  not implemented (returns -ENXIO)
* The support of async handlers can be built in via configure option,
  ``--enable-async``.  For simplicity, SALSA-lib supports only one async
  handler per PCM handler.

### CONTROL

* Supports only the hw layer, no plug-in
* Some H-control functions are not included
* The support of async handlers via ``--enable-async`` as well as PCM
  async handlers.

### MIXER

* No sort with weight as default, sorted in the order of numid
* Simple-none layer only
* "Capture Source" isn't split to switches but handled as an enum
   (I personally like this better :-)
* dB support can be selected via configure option
* Linear <-> log dB conversion enabled via configure option

### TIMER

* No query interface (returns errors)
* No async handler (returns -ENXIO)
* Disabled as default

### RAWMIDI

* Should work
* Disabled as default

### HWDEP

* Should work
* Disabled as default

### SEQUENCER

* All replaced with dummy functions.  The apps using sequencer won't
  work properly.
* Disabled as default

### INSTRUMENT LAYER

* No APIs are provided

### UCM

* Not supported yet at all

### MISC

* The API functions that are not supported but defined have
  *deprecated* attribute.  You'll get compile warnings when compiling
  the code using such functions.  (The same is true for the all
  components below.)  They are just warnings that you can usually
  ignore.  The warnings are given so that you can find the
  non-functional features easily.  If warnings really matter, remove
  the deprecated attribute via ``--disable-deprecated`` configure option.


INSTALLATION
------------

Some configure options are available to reduce the binary size.
As default, rawmidi, hwdep, timer, conf and seq components are
disabled.  They can be enabled via ``--enable-xxx`` option, such as
``--enable-rawmidi``.
The PCM and mixer components are enabled as default, and can be
disabled via ``--disable-pcm`` and ``--disable-mixer`` option, respectively.

The dB range support can be enabled via ``--enable-tlv`` option.  It's
disabled as default.  For enabling the floating-point math calculation
used in dB <-> linear conversion for the mixer, pass ``--enable-float``
option as well.

The support for user-space control elements is enabled as default
to keep the compatibility with the older salsa-lib releases.  But now
it can be disabled via ``--disable-user-elem`` configure option, too.

The async handler support can be enabled via ``--enable-async`` option.
It's disabled as default.

The PCM chmap API support is also optional, can be enabled via
``--enable-chmap`` option.

With option ``--enable-abi-compat``, libasound.so will be created as an
opt-in ABI-compatible library with the genuine ALSA-lib.

When ``--enable-abicheck`` is given, each open function checks the ABI
compatibility with the installed library.

When ``--enable-time64`` is passed, the library enforces to use the
64bit timespec and 64bit-compatible PCM ioctls even if the libc is
still with 32bit time.  It's good for testing the 64bit time support.

All these options can be enabled via a single option,
``--enable-everything``, for your convenience.

For disabling the deprecated attribute for non-working functions (see
the description of "MISC" in the section "GENERAL"), pass
``--disable-deprecated`` option.

For enabling the support for the string buffer with ``snd_output_*()``
function (e.g. used by PulseAudio), pass ``--enable-output-buffer`` option.

The alsa-library version to be compatible can be given explicitly via
``--with-compat-version option``.  As default, 1.2.5 is set.

If the ALSA device files are not in /dev/snd, another directory can be
specified with the ``--with-alsa-devdir option``.

The library is installed as ``$LIBDIR/libsalsa.so.*``.
This package provides also the compatible alsa.m4 and alsa.pc.  If the
other packages can be autoreconf'ed, you'll be able to link libsalsa
as it is.

The C header files are installed into ``$INCLUDEDIR/alsa``.  The
alsa/asoundlib.h should be compatible with the normal alsa-lib's one.


CROSS-COMPILATION
-----------------

For compiling the library with a cross compiler, run like the
following:

	% CC=arm-linux-gcc \
	  ./configure --target=arm-linux --host=i686-linux

Don't forget to add ``-linux`` to the host option value.  Otherwise
configure script won't detect the host type correctly, and the shared
library won't be built properly.


DOCUMENTATION
-------------

See alsa-lib reference.  It's compatible!


KNOWN PROBLEMS
--------------

* Incomplete MMAP mode

  ``snd_pcm_mmap_*()`` are only partially implemented, and the mmap-mode
  is less tested, so far.

* When built without ``--enable-output-buffer``, the buffer output via
  ``snd_output_*()`` functions doesn't work.  And, the last ``close``
  argument in ``snd_input_stdio_attach()`` and ``snd_output_stdio_attach()``
  is ignored and the attached IO is always closed.


LICENSE
-------

Distributed under LGPL.  See COPYING file.

