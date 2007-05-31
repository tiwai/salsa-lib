#ifndef __ALSA_GLOBAL_H
#define __ALSA_GLOBAL_H

typedef struct snd_config snd_config_t;

/** Timestamp */
typedef struct timeval snd_timestamp_t;
/** Hi-res timestamp */
typedef struct timespec snd_htimestamp_t;

typedef struct sndrv_pcm_info snd_pcm_info_t;
typedef struct sndrv_hwdep_info snd_hwdep_info_t;
typedef struct sndrv_rawmidi_info snd_rawmidi_info_t;

typedef struct _snd_ctl snd_ctl_t;
typedef struct _snd_pcm snd_pcm_t;
typedef struct _snd_rawmidi snd_rawmidi_t;
typedef struct _snd_hwdep snd_hwdep_t;

#ifndef ATTRIBUTE_UNUSED
/** do not print warning (gcc) when function parameter is not used */
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

#endif /* __ALSA_GLOBAL_H */
