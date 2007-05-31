/*
 *  Control Interface
 *
 *  Copyright (c) 2007 by Takashi Iwai <tiwai@suse.de>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/poll.h>
#include "control.h"
#include "local.h"

int _snd_dev_get_device(const char *name, int *cardp, int *devp, int *subdevp)
{
	*cardp = 0;
	*devp = 0;
	*subdevp = -1;
	if (!strcmp(name, "hw") || !strcmp(name, "default"))
		return 0;
	if (sscanf(name, "default:%d", cardp) > 0)
		return 0;
	if (devp) {
		if (sscanf(name, "hw:%d,%d,%d", cardp, devp, subdevp) > 0)
			return 0;
	} else {
		if (sscanf(name, "hw:%d", cardp) > 0)
			return 0;
	}
	return -EINVAL;
}


int snd_ctl_open(snd_ctl_t **ctlp, const char *name, int mode)
{
	snd_ctl_t *ctl;
	char filename[32];
	int err, fmode, fd, card, ver;

	err = _snd_dev_get_device(name, &card, NULL, NULL);
	if (err < 0)
		return err;

	snprintf(filename, sizeof(filename), "%s/controlC%d",
		 DEVPATH, card);
	if (mode & SND_CTL_READONLY)
		fmode = O_RDONLY;
	else
		fmode = O_RDWR;
	if (mode & SND_CTL_NONBLOCK)
		fmode |= O_NONBLOCK;
	if (mode & SND_CTL_ASYNC)
		fmode |= O_ASYNC;

	fd = open(filename, fmode);
	if (fd < 0)
		return -errno;

	if (ioctl(fd, SNDRV_CTL_IOCTL_PVERSION, &ver) < 0) {
		err = -errno;
		close(fd);
		return err;
	}
#if 0 // VCHECK
	if (SNDRV_PROTOCOL_INCOMPATIBLE(ver, SNDRV_CTL_VERSION_MAX)) {
		close(fd);
		return -SND_ERROR_INCOMPATIBLE_VERSION;
	}
#endif

	ctl = calloc(1, sizeof(*ctl));
	if (!ctl) {
		close(fd);
		return -ENOMEM;
	}
	ctl->card = card;
	ctl->fd = fd;
	ctl->protocol = ver;
	ctl->pollfd.fd = fd;
	ctl->pollfd.events = POLLIN | POLLERR | POLLNVAL;

	*ctlp = ctl;
	return 0;
}

int snd_ctl_close(snd_ctl_t *ctl)
{
	int err;
#if 0 // ASYNC
	while (!list_empty(&ctl->async_handlers)) {
		snd_async_handler_t *h = list_entry(&ctl->async_handlers.next, snd_async_handler_t, hlist);
		snd_async_del_handler(h);
	}
#endif
	err = close(ctl->fd) < 0 ? -errno : 0;
	free(ctl);
	return err;
}

int snd_ctl_nonblock(snd_ctl_t *ctl, int nonblock)
{
	int flags = fcntl(ctl->fd, F_GETFD);

	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	fcntl(ctl->fd, F_SETFD, flags);
	return 0;
}

#if 0 // ASYNC
int snd_ctl_async(snd_ctl_t *ctl, int sig, pid_t pid)
{
	if (sig == 0)
		sig = SIGIO;
	if (pid == 0)
		pid = getpid();
	return ctl->ops->async(ctl, sig, pid);
}
#endif

int snd_ctl_elem_add_integer(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count, long min, long max, long step)
{
	snd_ctl_elem_info_t info;
	snd_ctl_elem_value_t val;
	unsigned int i;
	int err;

	memset(&info, 0, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_INTEGER;
	info.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
#ifdef HAVE_DB_SUPPORT
	info.access |= SNDRV_CTL_ELEM_ACCESS_TLV_READWRITE;
#endif
	info.count = count;
	info.value.integer.min = min;
	info.value.integer.max = max;
	info.value.integer.step = step;
	err = snd_ctl_elem_add(ctl, &info);
	if (err < 0)
		return err;
	memset(&val, 0, sizeof(val));
	val.id = *id;
	for (i = 0; i < count; i++)
		val.value.integer.value[i] = min;
	return snd_ctl_elem_write(ctl, &val);
}

int snd_ctl_elem_add_integer64(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			       unsigned int count, long long min, long long max,
			       long long step)
{
	snd_ctl_elem_info_t info;
	snd_ctl_elem_value_t val;
	unsigned int i;
	int err;

	memset(&info, 0, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_INTEGER64;
	info.count = count;
	info.value.integer64.min = min;
	info.value.integer64.max = max;
	info.value.integer64.step = step;
	err = snd_ctl_elem_add(ctl, &info);
	if (err < 0)
		return err;
	memset(&val, 0, sizeof(val));
	val.id = *id;
	for (i = 0; i < count; i++)
		val.value.integer64.value[i] = min;
	return snd_ctl_elem_write(ctl, &val);
}

int snd_ctl_elem_add_boolean(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count)
{
	snd_ctl_elem_info_t info;

	memset(&info, 0, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_BOOLEAN;
	info.count = count;
	info.value.integer.min = 0;
	info.value.integer.max = 1;
	return snd_ctl_elem_add(ctl, &info);
}

int snd_ctl_elem_add_iec958(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id)
{
	snd_ctl_elem_info_t info;

	memset(&info, 0, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_IEC958;
	info.count = 1;
	return snd_ctl_elem_add(ctl, &info);
}

#ifdef HAVE_DB_SUPPORT
static int hw_elem_tlv(snd_ctl_t *handle, int op_flag,
		       unsigned int numid,
		       unsigned int *tlv, unsigned int tlv_size)
{
	int inum;
	struct sndrv_ctl_tlv *xtlv;
	
	/* we don't support TLV on protocol ver 2.0.3 or earlier */
	if (ctl->protocol < SNDRV_PROTOCOL_VERSION(2, 0, 4))
		return -ENXIO;

	switch (op_flag) {
	case -1: inum = SNDRV_CTL_IOCTL_TLV_COMMAND; break;
 	case 0:	inum = SNDRV_CTL_IOCTL_TLV_READ; break;
	case 1:	inum = SNDRV_CTL_IOCTL_TLV_WRITE; break;
	default: return -EINVAL;
	}
	xtlv = malloc(sizeof(struct sndrv_ctl_tlv) + tlv_size);
	if (xtlv == NULL)
		return -ENOMEM; 
	xtlv->numid = numid;
	xtlv->length = tlv_size;
	memcpy(xtlv->tlv, tlv, tlv_size);
	if (ioctl(hw->fd, inum, xtlv) < 0) {
		free(xtlv);
		return -errno;
	}
	if (op_flag == 0) {
		if (xtlv->tlv[1] + 2 * sizeof(unsigned int) > tlv_size)
			return -EFAULT;
		memcpy(tlv, xtlv->tlv, xtlv->tlv[1] + 2 * sizeof(unsigned int));
	}
	free(xtlv);
	return 0;
}

static int snd_ctl_tlv_do(snd_ctl_t *ctl, int op_flag,
			  const snd_ctl_elem_id_t *id,
		          unsigned int *tlv, unsigned int tlv_size)
{
	snd_ctl_elem_info_t info;
	int err;

	if (id->numid == 0) {
		memset(&info, 0, sizeof(info));
		info.id = *id;
		id = &info.id;
		err = snd_ctl_elem_info(ctl, &info);
		if (err < 0)
			goto __err;
		if (id->numid == 0) {
			err = -ENOENT;
			goto __err;
		}
	}
	err = hw_elem_tlv(ctl, op_flag, id->numid, tlv, tlv_size);
      __err:
      	if (info)
      		free(info);
	return err;
}

int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size)
{
	int err;
	if (tlv_size < 2 * sizeof(int))
		return -EINVAL;
	tlv[0] = -1;
	tlv[1] = 0;
	err = snd_ctl_tlv_do(ctl, 0, id, tlv, tlv_size);
	if (err >= 0 && tlv[0] == (unsigned int)-1)
		err = -ENXIO;
	return err;
}

int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv)
{
	return snd_ctl_tlv_do(ctl, 1, id,
			      (unsigned int *)tlv,
			      tlv[1] + 2 * sizeof(unsigned int));
}

int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv)
{
	return snd_ctl_tlv_do(ctl, -1, id,
			      (unsigned int *)tlv,
			      tlv[1] + 2 * sizeof(unsigned int));
}
#endif /* HAVE_DB_SUPPORT */

int snd_ctl_wait(snd_ctl_t *ctl, int timeout)
{
	struct pollfd pfd;
	int err;

	pfd = ctl->pollfd;
	for (;;) {
		err = poll(&pfd, 1, timeout);
		if (err < 0)
			return -errno;
		if (!err)
			return 0;
		if (pfd.revents & (POLLERR | POLLNVAL))
			return -EIO;
		if (pfd.revents & (POLLIN | POLLOUT))
			return 1;
	}
}

#if 0 // ASYNC
int snd_async_add_ctl_handler(snd_async_handler_t **handler, snd_ctl_t *ctl, 
			      snd_async_callback_t callback, void *private_data)
{
	int err;
	int was_empty;
	snd_async_handler_t *h;
	err = snd_async_add_handler(&h, _snd_ctl_async_descriptor(ctl),
				    callback, private_data);
	if (err < 0)
		return err;
	h->type = SND_ASYNC_HANDLER_CTL;
	h->u.ctl = ctl;
	was_empty = list_empty(&ctl->async_handlers);
	list_add_tail(&h->hlist, &ctl->async_handlers);
	if (was_empty) {
		err = snd_ctl_async(ctl, snd_async_handler_get_signo(h), getpid());
		if (err < 0) {
			snd_async_del_handler(h);
			return err;
		}
	}
	*handler = h;
	return 0;
}

snd_ctl_t *snd_async_handler_get_ctl(snd_async_handler_t *handler)
{
	assert(handler->type == SND_ASYNC_HANDLER_CTL);
	return handler->u.ctl;
}
#endif

#define TYPE(v) [SND_CTL_ELEM_TYPE_##v] = #v
#define IFACE(v) [SND_CTL_ELEM_IFACE_##v] = #v
#define IFACE1(v, n) [SND_CTL_ELEM_IFACE_##v] = #n
#define EVENT(v) [SND_CTL_EVENT_##v] = #v

const char *_snd_ctl_elem_type_names[] = {
	TYPE(NONE),
	TYPE(BOOLEAN),
	TYPE(INTEGER),
	TYPE(ENUMERATED),
	TYPE(BYTES),
	TYPE(IEC958),
	TYPE(INTEGER64),
};

const char *_snd_ctl_elem_iface_names[] = {
	IFACE(CARD),
	IFACE(HWDEP),
	IFACE(MIXER),
	IFACE(PCM),
	IFACE(RAWMIDI),
	IFACE(TIMER),
	IFACE(SEQUENCER),
};

const char *_snd_ctl_event_type_names[] = {
	EVENT(ELEM),
};
