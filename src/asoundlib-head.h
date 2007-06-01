#ifndef __ASOUNDLIB_H
#define __ASOUNDLIB_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <endian.h>
#include <sys/poll.h>
#include <errno.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "recipe.h"
#include "asoundef.h"
#include "version.h"
#include "global.h"
#include "input.h"
#include "output.h"
#include "error.h"
#include "control.h"
#include "pcm.h"
