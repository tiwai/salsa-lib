#ifndef __ALSA_HWDEP_H
#define __ALSA_HWDEP_H

#include "recipe.h"
#include "global.h"
#include "hwdep_types.h"

int snd_hwdep_open(snd_hwdep_t **hwdep, const char *name, int mode);
int snd_hwdep_close(snd_hwdep_t *hwdep);

#include "hwdep_macros.h"

#define snd_hwdep_info_alloca(ptr) do { *ptr = alloca(snd_hwdep_info_sizeof()); memset(*ptr, 0, snd_hwdep_info_sizeof()); } while (0)

#define snd_hwdep_dsp_status_alloca(ptr) do { *ptr = alloca(snd_hwdep_dsp_status_sizeof()); memset(*ptr, 0, snd_hwdep_dsp_status_sizeof()); } while (0)

#define snd_hwdep_dsp_image_alloca(ptr) do { *ptr = alloca(snd_hwdep_dsp_image_sizeof()); memset(*ptr, 0, snd_hwdep_dsp_image_sizeof()); } while (0)

#endif /* __ALSA_HWDEP_H */

