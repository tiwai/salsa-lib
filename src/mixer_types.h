#ifndef __ALSA_MIXER_TYPES_H
#define __ALSA_MIXER_TYPES_H

#include "pcm.h"

typedef struct _snd_mixer snd_mixer_t;
typedef struct _snd_mixer_elem snd_mixer_elem_t;
#define snd_mixer_class_t	snd_mixer_t

typedef int (*snd_mixer_callback_t)(snd_mixer_t *ctl,
				    unsigned int mask,
				    snd_mixer_elem_t *elem);

typedef int (*snd_mixer_elem_callback_t)(snd_mixer_elem_t *elem,
					 unsigned int mask);

typedef int (*snd_mixer_compare_t)(const snd_mixer_elem_t *e1,
				   const snd_mixer_elem_t *e2);

typedef enum _snd_mixer_elem_type {
	SND_MIXER_ELEM_SIMPLE,
	SND_MIXER_ELEM_LAST = SND_MIXER_ELEM_SIMPLE
} snd_mixer_elem_type_t;

typedef enum _snd_mixer_selem_channel_id {
	SND_MIXER_SCHN_UNKNOWN = -1,
	SND_MIXER_SCHN_FRONT_LEFT = 0,
	SND_MIXER_SCHN_FRONT_RIGHT,
	SND_MIXER_SCHN_REAR_LEFT,
	SND_MIXER_SCHN_REAR_RIGHT,
	SND_MIXER_SCHN_FRONT_CENTER,
	SND_MIXER_SCHN_WOOFER,
	SND_MIXER_SCHN_SIDE_LEFT,
	SND_MIXER_SCHN_SIDE_RIGHT,
	SND_MIXER_SCHN_REAR_CENTER,
	SND_MIXER_SCHN_LAST = 31,
	SND_MIXER_SCHN_MONO = SND_MIXER_SCHN_FRONT_LEFT
} snd_mixer_selem_channel_id_t;

enum snd_mixer_selem_regopt_abstract {
	SND_MIXER_SABSTRACT_NONE = 0,
	SND_MIXER_SABSTRACT_BASIC,
};

struct snd_mixer_selem_regopt {
	int ver;
	enum snd_mixer_selem_regopt_abstract abstract;
	const char *device;
	snd_pcm_t *playback_pcm;
	snd_pcm_t *capture_pcm;
};

typedef struct _snd_mixer_selem_id snd_mixer_selem_id_t;

#endif /* __ALSA_MIXER_TYPES_H */
