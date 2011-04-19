#include "recipe.h"
#include <errno.h>

/* async helpers */
typedef struct _snd_async_handler snd_async_handler_t;
typedef void (*snd_async_callback_t)(snd_async_handler_t *handler);

#if !SALSA_HAS_ASYNC_SUPPORT

int snd_async_add_handler(snd_async_handler_t **handler, int fd, 
			  snd_async_callback_t callback, void *private_data)
{
	return -ENXIO;
}

int snd_async_del_handler(snd_async_handler_t *handler)
{
	return -ENXIO;
}

#endif /* !SALSA_HAS_ASYNC_SUPPORT */

#if !SALSA_HAS_DUMMY_CONF
int snd_config_update_free_global(void)
{
	return 0;
}
#endif /* !SALSA_HAS_DUMMY_CONF */
