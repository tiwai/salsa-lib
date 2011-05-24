#ifndef __ALSA_CTL_TYPES_H
#define __ALSA_CTL_TYPES_H

#include <unistd.h>
#include <stdint.h>

/* TLV types */
#define SND_CTL_TLVT_CONTAINER		0
#define SND_CTL_TLVT_DB_SCALE		1
#define SND_CTL_TLVT_DB_LINEAR		2
#define SND_CTL_TLVT_DB_RANGE		3
#define SND_CTL_TLVT_DB_MINMAX		4
#define SND_CTL_TLVT_DB_MINMAX_MUTE	5

/* Mute state */
#define SND_CTL_TLV_DB_GAIN_MUTE	-9999999

typedef enum _snd_ctl_type {
	SND_CTL_TYPE_HW,
	SND_CTL_TYPE_SHM,	/* not used by SALSA */
	SND_CTL_TYPE_INET,	/* not used by SALSA */
	SND_CTL_TYPE_EXT	/* not used by SALSA */
} snd_ctl_type_t;

#define SND_CTL_NONBLOCK		0x0001
#define SND_CTL_ASYNC			0x0002
#define SND_CTL_READONLY		0x0004

#endif /* __ALSA_CTL_TYPES_H */
