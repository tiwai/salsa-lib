#ifndef __ALSA_CONTROL_H
#define __ALSA_CONTROL_H

/** IEC958 structure */
typedef struct snd_aes_iec958 {
	unsigned char status[24];	/**< AES/IEC958 channel status bits */
	unsigned char subcode[147];	/**< AES/IEC958 subcode bits */
	unsigned char pad;		/**< nothing */
	unsigned char dig_subframe[4];	/**< AES/IEC958 subframe bits */
} snd_aes_iec958_t;

/** CTL card info container */
typedef struct _snd_ctl_card_info snd_ctl_card_info_t;

/** CTL element identifier container */
typedef struct _snd_ctl_elem_id snd_ctl_elem_id_t;

/** CTL element identifier list container */
typedef struct _snd_ctl_elem_list snd_ctl_elem_list_t;

/** CTL element info container */
typedef struct _snd_ctl_elem_info snd_ctl_elem_info_t;

/** CTL element value container */
typedef struct _snd_ctl_elem_value snd_ctl_elem_value_t;

/** CTL event container */
typedef struct _snd_ctl_event snd_ctl_event_t;

/** CTL element type */
typedef enum _snd_ctl_elem_type {
	/** Invalid type */
	SND_CTL_ELEM_TYPE_NONE = 0,
	/** Boolean contents */
	SND_CTL_ELEM_TYPE_BOOLEAN,
	/** Integer contents */
	SND_CTL_ELEM_TYPE_INTEGER,
	/** Enumerated contents */
	SND_CTL_ELEM_TYPE_ENUMERATED,
	/** Bytes contents */
	SND_CTL_ELEM_TYPE_BYTES,
	/** IEC958 (S/PDIF) setting content */
	SND_CTL_ELEM_TYPE_IEC958,
	/** 64-bit integer contents */
	SND_CTL_ELEM_TYPE_INTEGER64,
	SND_CTL_ELEM_TYPE_LAST = SND_CTL_ELEM_TYPE_INTEGER64
} snd_ctl_elem_type_t;

/** CTL related interface */
typedef enum _snd_ctl_elem_iface {
	/** Card level */
	SND_CTL_ELEM_IFACE_CARD = 0,
	/** Hardware dependent device */
	SND_CTL_ELEM_IFACE_HWDEP,
	/** Mixer */
	SND_CTL_ELEM_IFACE_MIXER,
	/** PCM */
	SND_CTL_ELEM_IFACE_PCM,
	/** RawMidi */
	SND_CTL_ELEM_IFACE_RAWMIDI,
	/** Timer */
	SND_CTL_ELEM_IFACE_TIMER,
	/** Sequencer */
	SND_CTL_ELEM_IFACE_SEQUENCER,
	SND_CTL_ELEM_IFACE_LAST = SND_CTL_ELEM_IFACE_SEQUENCER
} snd_ctl_elem_iface_t;

/** Event class */
typedef enum _snd_ctl_event_type {
	/** Elements related event */
	SND_CTL_EVENT_ELEM = 0,
	SND_CTL_EVENT_LAST = SND_CTL_EVENT_ELEM
}snd_ctl_event_type_t;

/** Element has been removed (Warning: test this first and if set don't
  * test the other masks) \hideinitializer */
#define SND_CTL_EVENT_MASK_REMOVE 	(~0U)
/** Element value has been changed \hideinitializer */
#define SND_CTL_EVENT_MASK_VALUE	(1<<0)
/** Element info has been changed \hideinitializer */
#define SND_CTL_EVENT_MASK_INFO		(1<<1)
/** Element has been added \hideinitializer */
#define SND_CTL_EVENT_MASK_ADD		(1<<2)
/** Element's TLV value has been changed \hideinitializer */
#define SND_CTL_EVENT_MASK_TLV		(1<<3)

/** CTL name helper */
#define SND_CTL_NAME_NONE				""
/** CTL name helper */
#define SND_CTL_NAME_PLAYBACK				"Playback "
/** CTL name helper */
#define SND_CTL_NAME_CAPTURE				"Capture "

/** CTL name helper */
#define SND_CTL_NAME_IEC958_NONE			""
/** CTL name helper */
#define SND_CTL_NAME_IEC958_SWITCH			"Switch"
/** CTL name helper */
#define SND_CTL_NAME_IEC958_VOLUME			"Volume"
/** CTL name helper */
#define SND_CTL_NAME_IEC958_DEFAULT			"Default"
/** CTL name helper */
#define SND_CTL_NAME_IEC958_MASK			"Mask"
/** CTL name helper */
#define SND_CTL_NAME_IEC958_CON_MASK			"Con Mask"
/** CTL name helper */
#define SND_CTL_NAME_IEC958_PRO_MASK			"Pro Mask"
/** CTL name helper */
#define SND_CTL_NAME_IEC958_PCM_STREAM			"PCM Stream"
/** Element name for IEC958 (S/PDIF) */
#define SND_CTL_NAME_IEC958(expl,direction,what)	"IEC958 " expl SND_CTL_NAME_##direction SND_CTL_NAME_IEC958_##what

/** Mask for the major Power State identifier */
#define SND_CTL_POWER_MASK		0xff00
/** ACPI/PCI Power State D0 */
#define SND_CTL_POWER_D0          	0x0000
/** ACPI/PCI Power State D1 */
#define SND_CTL_POWER_D1     	     	0x0100
/** ACPI/PCI Power State D2 */
#define SND_CTL_POWER_D2 	        0x0200
/** ACPI/PCI Power State D3 */
#define SND_CTL_POWER_D3         	0x0300
/** ACPI/PCI Power State D3hot */
#define SND_CTL_POWER_D3hot		(SND_CTL_POWER_D3|0x0000)
/** ACPI/PCI Power State D3cold */
#define SND_CTL_POWER_D3cold	      	(SND_CTL_POWER_D3|0x0001)

/** TLV type - Container */
#define SND_CTL_TLVT_CONTAINER		0x0000
/** TLV type - basic dB scale */
#define SND_CTL_TLVT_DB_SCALE		0x0001
/** TLV type - linear volume */
#define SND_CTL_TLVT_DB_LINEAR		0x0002
/** TLV type - dB range container */
#define SND_CTL_TLVT_DB_RANGE		0x0003

/** Mute state */
#define SND_CTL_TLV_DB_GAIN_MUTE	-9999999

/** CTL type */
typedef enum _snd_ctl_type {
	/** Kernel level CTL */
	SND_CTL_TYPE_HW,
	/** Shared memory client CTL */
	SND_CTL_TYPE_SHM,
	/** INET client CTL (not yet implemented) */
	SND_CTL_TYPE_INET,
	/** External control plugin */
	SND_CTL_TYPE_EXT
} snd_ctl_type_t;

/** Non blocking mode (flag for open mode) \hideinitializer */
#define SND_CTL_NONBLOCK		0x0001

/** Async notification (flag for open mode) \hideinitializer */
#define SND_CTL_ASYNC			0x0002

/** Read only (flag for open mode) \hideinitializer */
#define SND_CTL_READONLY		0x0004

/** CTL handle */
typedef struct _snd_ctl snd_ctl_t;

int snd_card_load(int card);
int snd_card_next(int *card);
int snd_card_get_index(const char *name);
int snd_card_get_name(int card, char **name);
int snd_card_get_longname(int card, char **name);

int snd_device_name_hint(int card, const char *iface, void ***hints);
int snd_device_name_free_hint(void **hints);
char *snd_device_name_get_hint(const void *hint, const char *id);

int snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode);
int snd_ctl_close(snd_ctl_t *ctl);
int snd_ctl_nonblock(snd_ctl_t *ctl, int nonblock);
int snd_async_add_ctl_handler(snd_async_handler_t **handler, snd_ctl_t *ctl, 
			      snd_async_callback_t callback, void *private_data);
snd_ctl_t *snd_async_handler_get_ctl(snd_async_handler_t *handler);

int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size);
int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv);
int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv);

#include "ctl_macros.h"

#define snd_ctl_elem_id_alloca(ptr) do { *ptr = (snd_ctl_elem_id_t *) alloca(snd_ctl_elem_id_sizeof()); memset(*ptr, 0, snd_ctl_elem_id_sizeof()); } while (0)
#define snd_ctl_card_info_alloca(ptr) do { *ptr = (snd_ctl_card_info_t *) alloca(snd_ctl_card_info_sizeof()); memset(*ptr, 0, snd_ctl_card_info_sizeof()); } while (0)
#define snd_ctl_event_alloca(ptr) do { *ptr = (snd_ctl_event_t *) alloca(snd_ctl_event_sizeof()); memset(*ptr, 0, snd_ctl_event_sizeof()); } while (0)
#define snd_ctl_elem_list_alloca(ptr) do { *ptr = (snd_ctl_elem_list_t *) alloca(snd_ctl_elem_list_sizeof()); memset(*ptr, 0, snd_ctl_elem_list_sizeof()); } while (0)
#define snd_ctl_elem_info_alloca(ptr) do { *ptr = (snd_ctl_elem_info_t *) alloca(snd_ctl_elem_info_sizeof()); memset(*ptr, 0, snd_ctl_elem_info_sizeof()); } while (0)
#define snd_ctl_elem_value_alloca(ptr) do { *ptr = (snd_ctl_elem_value_t *) alloca(snd_ctl_elem_value_sizeof()); memset(*ptr, 0, snd_ctl_elem_value_sizeof()); } while (0)

#endif /* __ALSA_CONTROL_H */
