
/** HCTL element handle */
typedef struct _snd_hctl_elem snd_hctl_elem_t;

/** HCTL handle */
typedef struct _snd_hctl snd_hctl_t;

/**
 * \brief Compare function for sorting HCTL elements
 * \param e1 First element
 * \param e2 Second element
 * \return -1 if e1 < e2, 0 if e1 == e2, 1 if e1 > e2
 */
typedef int (*snd_hctl_compare_t)(const snd_hctl_elem_t *e1,
				  const snd_hctl_elem_t *e2);
int snd_hctl_compare_fast(const snd_hctl_elem_t *c1,
			  const snd_hctl_elem_t *c2);
/** 
 * \brief HCTL callback function
 * \param hctl HCTL handle
 * \param mask event mask
 * \param elem related HCTL element (if any)
 * \return 0 on success otherwise a negative error code
 */
typedef int (*snd_hctl_callback_t)(snd_hctl_t *hctl,
				   unsigned int mask,
				   snd_hctl_elem_t *elem);
/** 
 * \brief HCTL element callback function
 * \param elem HCTL element
 * \param mask event mask
 * \return 0 on success otherwise a negative error code
 */
typedef int (*snd_hctl_elem_callback_t)(snd_hctl_elem_t *elem,
					unsigned int mask);

int snd_hctl_open(snd_hctl_t **hctl, const char *name, int mode);
int snd_hctl_open_ctl(snd_hctl_t **hctlp, snd_ctl_t *ctl);
int snd_hctl_close(snd_hctl_t *hctl);
int snd_hctl_nonblock(snd_hctl_t *hctl, int nonblock);
int snd_hctl_poll_descriptors_count(snd_hctl_t *hctl);
int snd_hctl_poll_descriptors(snd_hctl_t *hctl, struct pollfd *pfds, unsigned int space);
int snd_hctl_poll_descriptors_revents(snd_hctl_t *ctl, struct pollfd *pfds, unsigned int nfds, unsigned short *revents);
unsigned int snd_hctl_get_count(snd_hctl_t *hctl);
int snd_hctl_set_compare(snd_hctl_t *hctl, snd_hctl_compare_t hsort);
snd_hctl_elem_t *snd_hctl_first_elem(snd_hctl_t *hctl);
snd_hctl_elem_t *snd_hctl_last_elem(snd_hctl_t *hctl);
snd_hctl_elem_t *snd_hctl_find_elem(snd_hctl_t *hctl, const snd_ctl_elem_id_t *id);
void snd_hctl_set_callback(snd_hctl_t *hctl, snd_hctl_callback_t callback);
void snd_hctl_set_callback_private(snd_hctl_t *hctl, void *data);
void *snd_hctl_get_callback_private(snd_hctl_t *hctl);
int snd_hctl_load(snd_hctl_t *hctl);
int snd_hctl_free(snd_hctl_t *hctl);
int snd_hctl_handle_events(snd_hctl_t *hctl);
const char *snd_hctl_name(snd_hctl_t *hctl);
int snd_hctl_wait(snd_hctl_t *hctl, int timeout);
snd_ctl_t *snd_hctl_ctl(snd_hctl_t *hctl);

snd_hctl_elem_t *snd_hctl_elem_next(snd_hctl_elem_t *elem);
snd_hctl_elem_t *snd_hctl_elem_prev(snd_hctl_elem_t *elem);
int snd_hctl_elem_info(snd_hctl_elem_t *elem, snd_ctl_elem_info_t * info);
int snd_hctl_elem_read(snd_hctl_elem_t *elem, snd_ctl_elem_value_t * value);
int snd_hctl_elem_write(snd_hctl_elem_t *elem, snd_ctl_elem_value_t * value);
int snd_hctl_elem_tlv_read(snd_hctl_elem_t *elem, unsigned int *tlv, unsigned int tlv_size);
int snd_hctl_elem_tlv_write(snd_hctl_elem_t *elem, const unsigned int *tlv);
int snd_hctl_elem_tlv_command(snd_hctl_elem_t *elem, const unsigned int *tlv);

snd_hctl_t *snd_hctl_elem_get_hctl(snd_hctl_elem_t *elem);

void snd_hctl_elem_get_id(const snd_hctl_elem_t *obj, snd_ctl_elem_id_t *ptr);
unsigned int snd_hctl_elem_get_numid(const snd_hctl_elem_t *obj);
snd_ctl_elem_iface_t snd_hctl_elem_get_interface(const snd_hctl_elem_t *obj);
unsigned int snd_hctl_elem_get_device(const snd_hctl_elem_t *obj);
unsigned int snd_hctl_elem_get_subdevice(const snd_hctl_elem_t *obj);
const char *snd_hctl_elem_get_name(const snd_hctl_elem_t *obj);
unsigned int snd_hctl_elem_get_index(const snd_hctl_elem_t *obj);
void snd_hctl_elem_set_callback(snd_hctl_elem_t *obj, snd_hctl_elem_callback_t val);
void * snd_hctl_elem_get_callback_private(const snd_hctl_elem_t *obj);
void snd_hctl_elem_set_callback_private(snd_hctl_elem_t *obj, void * val);
