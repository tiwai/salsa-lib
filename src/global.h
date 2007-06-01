#ifndef __ALSA_GLOBAL_H
#define __ALSA_GLOBAL_H

typedef struct snd_config snd_config_t;
typedef struct _snd_async_handler snd_async_handler_t;
typedef void (*snd_async_callback_t)(snd_async_handler_t *handler);

#if !defined(_POSIX_C_SOURCE) && !defined(_POSIX_SOURCE)
struct timeval {
	time_t		tv_sec;		/* seconds */
	long		tv_usec;	/* microseconds */
};

struct timespec {
	time_t		tv_sec;		/* seconds */
	long		tv_nsec;	/* nanoseconds */
};
#endif

typedef struct timeval snd_timestamp_t;
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

int _snd_set_nonblock(int fd, int nonblock);

#endif /* __ALSA_GLOBAL_H */
