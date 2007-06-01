#ifndef __ALSA_TIMER_H
#define __ALSA_TIMER_H

#include "recipe.h"
#include "global.h"
#include "timer_types.h"


int snd_timer_open(snd_timer_t **handle, const char *name, int mode);
int snd_timer_close(snd_timer_t *handle);
#if 0 // ASYNC
int snd_async_add_timer_handler(snd_async_handler_t **handler, snd_timer_t *timer,
				snd_async_callback_t callback, void *private_data);
snd_timer_t *snd_async_handler_get_timer(snd_async_handler_t *handler);
#endif

#include "timer_macros.h"

#define snd_timer_id_alloca(ptr) do { *ptr = alloca(snd_timer_id_sizeof()); memset(*ptr, 0, snd_timer_id_sizeof()); } while (0)

#define snd_timer_ginfo_alloca(ptr) do { *ptr = alloca(snd_timer_ginfo_sizeof()); memset(*ptr, 0, snd_timer_ginfo_sizeof()); } while (0)

#define snd_timer_info_alloca(ptr) do { *ptr = alloca(snd_timer_info_sizeof()); memset(*ptr, 0, snd_timer_info_sizeof()); } while (0)

#define snd_timer_params_alloca(ptr) do { *ptr = alloca(snd_timer_params_sizeof()); memset(*ptr, 0, snd_timer_params_sizeof()); } while (0)

#define snd_timer_status_alloca(ptr) do { *ptr = alloca(snd_timer_status_sizeof()); memset(*ptr, 0, snd_timer_status_sizeof()); } while (0)


#endif /** __ALSA_TIMER_H */
