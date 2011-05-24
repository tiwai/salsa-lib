#ifndef __ALSA_RAWMIDI_TYPES_H
#define __ALSA_RAWMIDI_TYPES_H

#define SND_RAWMIDI_APPEND	0x0001
#define SND_RAWMIDI_NONBLOCK	0x0002
#define SND_RAWMIDI_SYNC	0x0004

typedef enum _snd_rawmidi_type {
	SND_RAWMIDI_TYPE_HW,
	SND_RAWMIDI_TYPE_SHM,		/* not used by SALSA */
	SND_RAWMIDI_TYPE_INET,		/* not used by SALSA */
	SND_RAWMIDI_TYPE_VIRTUAL	/* not used by SALSA */
} snd_rawmidi_type_t;

#endif /* __ALSA_RAWMIDI_TYPES_H */
