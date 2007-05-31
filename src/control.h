#ifndef __ALSA_CONTROL_H
#define __ALSA_CONTROL_H

#include "global.h"
#include "ctl_types.h"

int snd_card_load(int card);
int snd_card_next(int *card);
int snd_card_get_index(const char *name);
int snd_card_get_name(int card, char **name);
int snd_card_get_longname(int card, char **name);

int snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode);
int snd_ctl_close(snd_ctl_t *ctl);
int snd_ctl_wait(snd_ctl_t *ctl, int timeout);
#if 0 // ASYNC
int snd_async_add_ctl_handler(snd_async_handler_t **handler, snd_ctl_t *ctl, 
			      snd_async_callback_t callback, void *private_data);
snd_ctl_t *snd_async_handler_get_ctl(snd_async_handler_t *handler);
#endif

int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size);
int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv);
int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv);

#include "ctl_macros.h"

#define snd_ctl_elem_id_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_id_sizeof()); memset(*ptr, 0, snd_ctl_elem_id_sizeof()); } while (0)
#define snd_ctl_card_info_alloca(ptr) do { *ptr = alloca(snd_ctl_card_info_sizeof()); memset(*ptr, 0, snd_ctl_card_info_sizeof()); } while (0)
#define snd_ctl_event_alloca(ptr) do { *ptr = alloca(snd_ctl_event_sizeof()); memset(*ptr, 0, snd_ctl_event_sizeof()); } while (0)
#define snd_ctl_elem_list_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_list_sizeof()); memset(*ptr, 0, snd_ctl_elem_list_sizeof()); } while (0)
#define snd_ctl_elem_info_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_info_sizeof()); memset(*ptr, 0, snd_ctl_elem_info_sizeof()); } while (0)
#define snd_ctl_elem_value_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_value_sizeof()); memset(*ptr, 0, snd_ctl_elem_value_sizeof()); } while (0)

#endif /* __ALSA_CONTROL_H */
