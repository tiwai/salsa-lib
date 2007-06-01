#ifndef __ALSA_RAWMIDI_H
#define __ALSA_RAWMIDI_H

#include "recipe.h"
#include "global.h"
#include "rawmidi_types.h"

int snd_rawmidi_open(snd_rawmidi_t **in_rmidi, snd_rawmidi_t **out_rmidi,
		     const char *name, int mode);
int snd_rawmidi_close(snd_rawmidi_t *rmidi);

#include "rawmidi_macros.h"

#define snd_rawmidi_info_alloca(ptr) do { *ptr = alloca(snd_rawmidi_info_sizeof()); memset(*ptr, 0, snd_rawmidi_info_sizeof()); } while (0)

#define snd_rawmidi_params_alloca(ptr) do { *ptr = alloca(snd_rawmidi_params_sizeof()); memset(*ptr, 0, snd_rawmidi_params_sizeof()); } while (0)

#define snd_rawmidi_status_alloca(ptr) do { *ptr = alloca(snd_rawmidi_status_sizeof()); memset(*ptr, 0, snd_rawmidi_status_sizeof()); } while (0)

#endif /* __RAWMIDI_H */

