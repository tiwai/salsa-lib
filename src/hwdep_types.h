#define SND_HWDEP_OPEN_READ		(O_RDONLY)
#define SND_HWDEP_OPEN_WRITE		(O_WRONLY)
#define SND_HWDEP_OPEN_DUPLEX		(O_RDWR)
#define SND_HWDEP_OPEN_NONBLOCK		(O_NONBLOCK)

typedef enum _snd_hwdep_type {
	SND_HWDEP_TYPE_HW,
	SND_HWDEP_TYPE_SHM,	/* not used by SALSA */
	SND_HWDEP_TYPE_INET	/* not used by SALSA */
} snd_hwdep_type_t;
