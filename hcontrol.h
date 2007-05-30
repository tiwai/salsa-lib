#ifndef __ALSA_HCONTROL_H
#define __ALSA_HCONTROL_H

#include "control.h"
#include "hctl_types.h"

int snd_hctl_open(snd_hctl_t **hctl, const char *name, int mode);
int snd_hctl_open_ctl(snd_hctl_t **hctlp, snd_ctl_t *ctl);
int snd_hctl_close(snd_hctl_t *hctl);
snd_hctl_elem_t *snd_hctl_find_elem(snd_hctl_t *hctl, const snd_ctl_elem_id_t *id);
int snd_hctl_load(snd_hctl_t *hctl);
int snd_hctl_free(snd_hctl_t *hctl);
int snd_hctl_handle_events(snd_hctl_t *hctl);

#include "hctl_macros.h"

#endif /* __ALSA_HCONTROL_H */
