#ifndef __ALSA_COMMON_TYPES_H
#define __ALSA_COMMON_TYPES_H

typedef struct snd_config snd_config_t;

/** Timestamp */
typedef struct timeval snd_timestamp_t;
/** Hi-res timestamp */
typedef struct timespec snd_htimestamp_t;

typedef struct sndrv_pcm_info snd_pcm_info_t;
typedef struct sndrv_hwdep_info snd_hwdep_info_t;
typedef struct sndrv_rawmidi_info snd_rawmidi_info_t;

#endif /* __ALSA_COMMON_TYPES_H */
