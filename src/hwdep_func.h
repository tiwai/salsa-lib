/*
 * SALSA-Lib - Hardware Dependent Interface
 * Declarations of exported functions
 */

#include "global.h"
#include "hwdep_types.h"

int snd_hwdep_open(snd_hwdep_t **hwdep, const char *name, int mode);
int snd_hwdep_close(snd_hwdep_t *hwdep);
