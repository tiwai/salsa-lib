/*
 */

#include "recipe.h"
#include "global.h"
#include "rawmidi_types.h"

int snd_rawmidi_open(snd_rawmidi_t **in_rmidi, snd_rawmidi_t **out_rmidi,
		     const char *name, int mode);
int snd_rawmidi_close(snd_rawmidi_t *rmidi);

