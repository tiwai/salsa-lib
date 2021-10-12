/*
 * SALSA-Lib - Output Macros
 * ABI-compatible definitions
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "recipe.h"
#include "global.h"
#undef __SALSA_EXPORT_FUNC
#define __SALSA_EXPORT_FUNC
#include "output.h"
