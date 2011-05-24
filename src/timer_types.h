#ifndef __ALSA_TIMER_TYPES_H
#define __ALSA_TIMER_TYPES_H

#define SND_TIMER_OPEN_NONBLOCK		(1<<0)
#define SND_TIMER_OPEN_TREAD		(1<<1)

typedef enum _snd_timer_type {
	SND_TIMER_TYPE_HW = 0,
	SND_TIMER_TYPE_SHM,	/* not used by SALSA */
	SND_TIMER_TYPE_INET	/* not used by SALSA */
} snd_timer_type_t;

typedef struct _snd_timer_query snd_timer_query_t;
typedef struct _snd_timer snd_timer_t;

#endif /* __ALSA_TIMER_TYPES_H */
