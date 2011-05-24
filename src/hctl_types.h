#ifndef __ALSA_HCTL_TYPES_H
#define __ALSA_HCTL_TYPES_H

typedef struct _snd_hctl_elem snd_hctl_elem_t;
typedef struct _snd_hctl snd_hctl_t;
typedef int (*snd_hctl_compare_t)(const snd_hctl_elem_t *e1,
				  const snd_hctl_elem_t *e2);
int snd_hctl_compare_fast(const snd_hctl_elem_t *c1,
			  const snd_hctl_elem_t *c2);
typedef int (*snd_hctl_callback_t)(snd_hctl_t *hctl,
				   unsigned int mask,
				   snd_hctl_elem_t *elem);
typedef int (*snd_hctl_elem_callback_t)(snd_hctl_elem_t *elem,
					unsigned int mask);

#endif /* __ALSA_HCTL_TYPES_H */
