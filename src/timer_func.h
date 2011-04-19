/*
 */

#include "recipe.h"
#include "global.h"
#include "hcontrol.h"
#include "timer_types.h"

int snd_timer_open(snd_timer_t **handle, const char *name, int mode);
int snd_timer_close(snd_timer_t *handle);
