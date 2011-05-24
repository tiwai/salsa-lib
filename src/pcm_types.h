#ifndef __ALSA_PCM_TYPES_H
#define __ALSA_PCM_TYPES_H

#include <unistd.h>
#include <stdint.h>

typedef snd_mask_t snd_pcm_access_mask_t;
typedef snd_mask_t snd_pcm_format_mask_t;
typedef snd_mask_t snd_pcm_subformat_mask_t;

/* deprecated */
typedef enum _snd_pcm_start {
	SND_PCM_START_DATA = 0,
	SND_PCM_START_EXPLICIT,
	SND_PCM_START_LAST = SND_PCM_START_EXPLICIT
} snd_pcm_start_t;

/* deprecated */
typedef enum _snd_pcm_xrun {
	SND_PCM_XRUN_NONE = 0,
	SND_PCM_XRUN_STOP,
	SND_PCM_XRUN_LAST = SND_PCM_XRUN_STOP
} snd_pcm_xrun_t;

#define SND_PCM_NONBLOCK		0x00000001
#define SND_PCM_ASYNC			0x00000002
#define SND_PCM_NO_AUTO_RESAMPLE	0x00010000
#define SND_PCM_NO_AUTO_CHANNELS	0x00020000
#define SND_PCM_NO_AUTO_FORMAT		0x00040000
#define SND_PCM_NO_SOFTVOL		0x00080000

typedef enum _snd_pcm_type {
	SND_PCM_TYPE_HW = 0,
	/* the rest are not supported by SALSA (of course!) */
	SND_PCM_TYPE_HOOKS,
	SND_PCM_TYPE_MULTI,
	SND_PCM_TYPE_FILE,
	SND_PCM_TYPE_NULL,
	SND_PCM_TYPE_SHM,
	SND_PCM_TYPE_INET,
	SND_PCM_TYPE_COPY,
	SND_PCM_TYPE_LINEAR,
	SND_PCM_TYPE_ALAW,
	SND_PCM_TYPE_MULAW,
	SND_PCM_TYPE_ADPCM,
	SND_PCM_TYPE_RATE,
	SND_PCM_TYPE_ROUTE,
	SND_PCM_TYPE_PLUG,
	SND_PCM_TYPE_SHARE,
	SND_PCM_TYPE_METER,
	SND_PCM_TYPE_MIX,
	SND_PCM_TYPE_DROUTE,
	SND_PCM_TYPE_LBSERVER,
	SND_PCM_TYPE_LINEAR_FLOAT,
	SND_PCM_TYPE_LADSPA,
	SND_PCM_TYPE_DMIX,
	SND_PCM_TYPE_JACK,
	SND_PCM_TYPE_DSNOOP,
	SND_PCM_TYPE_DSHARE,
	SND_PCM_TYPE_IEC958,
	SND_PCM_TYPE_SOFTVOL,
	SND_PCM_TYPE_IOPLUG,
	SND_PCM_TYPE_EXTPLUG,
	SND_PCM_TYPE_LAST = SND_PCM_TYPE_EXTPLUG
} snd_pcm_type_t;

typedef struct _snd_pcm_channel_area {
	void *addr;
	unsigned int first;
	unsigned int step;
} snd_pcm_channel_area_t;

#endif /* __ALSA_PCM_TYPES_H */
