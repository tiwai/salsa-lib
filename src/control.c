/*
 *  SALSA-Lib - Control Interface
 *
 *  Copyright (c) 2007-2011 by Takashi Iwai <tiwai@suse.de>
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
#if SALSA_SUPPORT_FLOAT
#include <math.h>
#endif


/*
 * open/close
 */

int snd_ctl_open(snd_ctl_t **ctlp, const char *name, int mode)
{
	snd_ctl_t *ctl;
	char filename[sizeof(SALSA_DEVPATH) + 24];
	int err, fmode, fd, card, ver;

	*ctlp = NULL;

	err = _snd_dev_get_device(name, &card, NULL, NULL);
	if (err < 0)
		return err;

	snprintf(filename, sizeof(filename), "%s/controlC%d",
		 SALSA_DEVPATH, card);
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
#if SALSA_HAS_ASYNC_SUPPORT
	if (ctl->async)
		snd_async_del_handler(ctl->async);
#endif
	close(ctl->fd);
	free(ctl);
	return 0;
}


/*
 * add/remove user-defined controls
 */

int snd_ctl_elem_add_integer(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count, long min, long max, long step)
{
	snd_ctl_elem_info_t info;
	snd_ctl_elem_value_t val;
	unsigned int i;
	int err;

	memzero_valgrind(&info, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_INTEGER;
	info.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
#if SALSA_HAS_TLV_SUPPORT
	info.access |= SNDRV_CTL_ELEM_ACCESS_TLV_READWRITE;
#endif
	info.count = count;
	info.value.integer.min = min;
	info.value.integer.max = max;
	info.value.integer.step = step;
	err = snd_ctl_elem_add(ctl, &info);
	if (err < 0)
		return err;
	memzero_valgrind(&val, sizeof(val));
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

	memzero_valgrind(&info, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_INTEGER64;
	info.count = count;
	info.value.integer64.min = min;
	info.value.integer64.max = max;
	info.value.integer64.step = step;
	err = snd_ctl_elem_add(ctl, &info);
	if (err < 0)
		return err;
	memzero_valgrind(&val, sizeof(val));
	val.id = *id;
	for (i = 0; i < count; i++)
		val.value.integer64.value[i] = min;
	return snd_ctl_elem_write(ctl, &val);
}

int snd_ctl_elem_add_boolean(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count)
{
	snd_ctl_elem_info_t info;

	memzero_valgrind(&info, sizeof(info));
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

	memzero_valgrind(&info, sizeof(info));
	info.id = *id;
	info.type = SND_CTL_ELEM_TYPE_IEC958;
	info.count = 1;
	return snd_ctl_elem_add(ctl, &info);
}


#if SALSA_HAS_TLV_SUPPORT
/*
 * TLV support
 */
static int hw_elem_tlv(snd_ctl_t *ctl, int inum,
		       unsigned int numid,
		       unsigned int *tlv, unsigned int tlv_size)
{
	snd_ctl_tlv_t *xtlv;
	
	/* we don't support TLV on protocol ver 2.0.3 or earlier */
	if (ctl->protocol < SNDRV_PROTOCOL_VERSION(2, 0, 4))
		return -ENXIO;

	xtlv = malloc(sizeof(snd_ctl_tlv_t) + tlv_size);
	if (xtlv == NULL)
		return -ENOMEM; 
	xtlv->numid = numid;
	xtlv->length = tlv_size;
	memcpy(xtlv->tlv, tlv, tlv_size);
	if (ioctl(ctl->fd, inum, xtlv) < 0) {
		free(xtlv);
		return -errno;
	}
	if (inum == SNDRV_CTL_IOCTL_TLV_READ) {
		if (xtlv->tlv[1] + 2 * sizeof(unsigned int) > tlv_size) {
			free(xtlv);
			return -EFAULT;
		}
		memcpy(tlv, xtlv->tlv, xtlv->tlv[1] + 2 * sizeof(unsigned int));
	}
	free(xtlv);
	return 0;
}

static int snd_ctl_tlv_do(snd_ctl_t *ctl, int cmd,
			  const snd_ctl_elem_id_t *id,
		          unsigned int *tlv, unsigned int tlv_size)
{
	if (!id->numid) {
		int err;
		snd_ctl_elem_info_t info;
		memzero_valgrind(&info, sizeof(info));
		info.id = *id;
		id = &info.id;
		err = snd_ctl_elem_info(ctl, &info);
		if (err < 0)
			return err;
		if (!id->numid)
			return -ENOENT;
	}
	return hw_elem_tlv(ctl, cmd, id->numid, tlv, tlv_size);
}

int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size)
{
	int err;
	if (tlv_size < 2 * sizeof(int))
		return -EINVAL;
	tlv[0] = -1;
	tlv[1] = 0;
	err = snd_ctl_tlv_do(ctl, SNDRV_CTL_IOCTL_TLV_READ, id, tlv, tlv_size);
	if (err >= 0 && tlv[0] == (unsigned int)-1)
		err = -ENXIO;
	return err;
}

int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv)
{
	return snd_ctl_tlv_do(ctl, SNDRV_CTL_IOCTL_TLV_WRITE, id,
			      (unsigned int *)tlv,
			      tlv[1] + 2 * sizeof(unsigned int));
}

int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv)
{
	return snd_ctl_tlv_do(ctl, SNDRV_CTL_IOCTL_TLV_COMMAND, id,
			      (unsigned int *)tlv,
			      tlv[1] + 2 * sizeof(unsigned int));
}
#endif /* SALSA_HAS_TLV_SUPPORT */


/*
 * Misc routines
 */
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

/*
 * strings
 */
#define TYPE(v) [SND_CTL_ELEM_TYPE_##v] = #v
#define IFACE(v) [SND_CTL_ELEM_IFACE_##v] = #v
#define IFACE1(v, n) [SND_CTL_ELEM_IFACE_##v] = #n
#define EVENT(v) [SND_CTL_EVENT_##v] = #v

const char * const _snd_ctl_elem_type_names[] = {
	TYPE(NONE),
	TYPE(BOOLEAN),
	TYPE(INTEGER),
	TYPE(ENUMERATED),
	TYPE(BYTES),
	TYPE(IEC958),
	TYPE(INTEGER64),
};

const char * const _snd_ctl_elem_iface_names[] = {
	IFACE(CARD),
	IFACE(HWDEP),
	IFACE(MIXER),
	IFACE(PCM),
	IFACE(RAWMIDI),
	IFACE(TIMER),
	IFACE(SEQUENCER),
};

const char * const _snd_ctl_event_type_names[] = {
	EVENT(ELEM),
};


#if SALSA_HAS_ASYNC_SUPPORT
/*
 * async helper
 */
int snd_async_add_ctl_handler(snd_async_handler_t **handler, snd_ctl_t *ctl, 
			      snd_async_callback_t callback,
			      void *private_data)
{
	int err;

	if (ctl->async)
		return -EBUSY;
	err = snd_async_add_handler(&ctl->async, ctl->fd,
				    callback, private_data);
	if (err < 0)
		return err;
	ctl->async->rec = ctl;
	ctl->async->pointer = &ctl->async;
	return 0;
}
#endif /* SALSA_HAS_ASYNC_SUPPORT */


#if SALSA_HAS_TLV_SUPPORT

/* convert to index of integer array */
#define int_index(size)	(((size) + sizeof(int) - 1) / sizeof(int))

/* max size of a TLV entry for dB information (including compound one) */
#define MAX_TLV_RANGE_SIZE	256

/* convert the given raw volume value to a dB gain
 */
static int tlv_to_dB_range(unsigned int *tlv, long rangemin, long rangemax,
			   long volume, long *db_gain)
{
	unsigned int pos, len;

	len = int_index(tlv[1]);
	if (len > MAX_TLV_RANGE_SIZE)
		return -EINVAL;
	pos = 2;
	while (pos + 4 <= len) {
		rangemin = (int)tlv[pos++];
		rangemax = (int)tlv[pos++];
		if (volume >= rangemin && volume <= rangemax)
			return snd_tlv_convert_to_dB(tlv + pos,
						     rangemin, rangemax,
						     volume, db_gain);
		pos++;
		pos += int_index(tlv[pos]) + 1;
	}
	return -EINVAL;
}

static int tlv_to_dB_scale(unsigned int *tlv, long rangemin, long rangemax,
			  long volume, long *db_gain)
{
	int min, step, mute;

	min = tlv[2];
	step = (tlv[3] & 0xffff);
	mute = (tlv[3] >> 16) & 1;
	if (mute && volume == rangemin)
		*db_gain = SND_CTL_TLV_DB_GAIN_MUTE;
	else
		*db_gain = (volume - rangemin) * step + min;
	return 0;
}

static int tlv_to_dB_minmax(unsigned int *tlv, long rangemin, long rangemax,
			   long volume, long *db_gain)
{
	int mindb, maxdb;

	mindb = tlv[2];
	maxdb = tlv[3];
	if (volume <= rangemin || rangemax <= rangemin) {
		if (tlv[0] == SND_CTL_TLVT_DB_MINMAX_MUTE)
			*db_gain = SND_CTL_TLV_DB_GAIN_MUTE;
		else
			*db_gain = mindb;
	} else if (volume >= rangemax)
		*db_gain = maxdb;
	else
		*db_gain = (maxdb - mindb) * (volume - rangemin) /
			(rangemax - rangemin) + mindb;
	return 0;
}

#if SALSA_SUPPORT_FLOAT
static int tlv_to_dB_linear(unsigned int *tlv, long rangemin, long rangemax,
			    long volume, long *db_gain)
{
	int mindb = tlv[2];
	int maxdb = tlv[3];

	if (volume <= rangemin || rangemax <= rangemin)
		*db_gain = mindb;
	else if (volume >= rangemax)
		*db_gain = maxdb;
	else {
		double val = (double)(volume - rangemin) /
			(double)(rangemax - rangemin);
		if (mindb <= SND_CTL_TLV_DB_GAIN_MUTE)
			*db_gain = (long)(100.0 * 20.0 * log10(val)) + maxdb;
		else {
			/* FIXME: precalculate and cache these values */
			double lmin = pow(10.0, mindb/2000.0);
			double lmax = pow(10.0, maxdb/2000.0);
			val = (lmax - lmin) * val + lmin;
			*db_gain = (long)(100.0 * 20.0 * log10(val));
		}
	}
	return 0;
}
#else
static inline int tlv_to_dB_linear(unsigned int *tlv, long rangemin,
				   long rangemax, long volume, long *db_gain)
{
	return -EINVAL;
}
#endif

int snd_tlv_convert_to_dB(unsigned int *tlv, long rangemin, long rangemax,
			  long volume, long *db_gain)
{
	switch (tlv[0]) {
	case SND_CTL_TLVT_DB_RANGE:
		return tlv_to_dB_range(tlv, rangemin, rangemax,
				       volume, db_gain);
	case SND_CTL_TLVT_DB_SCALE:
		return tlv_to_dB_scale(tlv, rangemin, rangemax, volume,
				       db_gain);
	case SND_CTL_TLVT_DB_MINMAX:
	case SND_CTL_TLVT_DB_MINMAX_MUTE:
		return tlv_to_dB_minmax(tlv, rangemin, rangemax, volume,
					db_gain);
	case SND_CTL_TLVT_DB_LINEAR:
		return tlv_to_dB_linear(tlv, rangemin, rangemax, volume,
					db_gain);
	}
	return -EINVAL;
}

/* Get the dB min/max values
 */
static int tlv_range_dB_range(unsigned int *tlv, long rangemin, long rangemax,
			      long *min, long *max)
{
	unsigned int pos, len;
	int err;

	len = int_index(tlv[1]);
	if (len > MAX_TLV_RANGE_SIZE)
		return -EINVAL;
	pos = 2;
	while (pos + 4 <= len) {
		long rmin, rmax;
		long submin, submax;
		submin = (int)tlv[pos++];
		submax = (int)tlv[pos++];
		if (rangemax < submax)
			submax = rangemax;
		err = snd_tlv_get_dB_range(tlv + pos, submin, submax,
					   &rmin, &rmax);
		if (err < 0)
			return err;
		if (pos > 4) {
			if (rmin < *min)
				*min = rmin;
			if (rmax > *max)
				*max = rmax;
		} else {
			*min = rmin;
			*max = rmax;
		}
		if (rangemax == submax)
			return 0;
		pos++;
		pos += int_index(tlv[pos]) + 1;
	}
	return 0;
}

static int tlv_range_dB_scale(unsigned int *tlv, long rangemin, long rangemax,
			      long *min, long *max)
{
	int step;
	if (tlv[3] & 0x10000)
		*min = SND_CTL_TLV_DB_GAIN_MUTE;
	else
		*min = (int)tlv[2];
	step = (tlv[3] & 0xffff);
	*max = (int)tlv[2] + step * (rangemax - rangemin);
	return 0;
}

int snd_tlv_get_dB_range(unsigned int *tlv, long rangemin, long rangemax,
			 long *min, long *max)
{
	switch (tlv[0]) {
	case SND_CTL_TLVT_DB_RANGE:
		return tlv_range_dB_range(tlv, rangemin, rangemax, min, max);
	case SND_CTL_TLVT_DB_SCALE:
		return tlv_range_dB_scale(tlv, rangemin, rangemax, min, max);
	case SND_CTL_TLVT_DB_MINMAX:
	case SND_CTL_TLVT_DB_LINEAR:
		*min = (int)tlv[2];
		*max = (int)tlv[3];
		return 0;
	case SND_CTL_TLVT_DB_MINMAX_MUTE:
		*min = SND_CTL_TLV_DB_GAIN_MUTE;
		*max = (int)tlv[3];
		return 0;
	}
	return -EINVAL;
}

/* Convert from dB gain to the corresponding raw value.
 * The value is round up when xdir > 0.
 */
static int tlv_from_dB_range(unsigned int *tlv, long rangemin, long rangemax,
			     long db_gain, long *value, int xdir)
{
	unsigned int pos, len;
	long dbmin, dbmax, prev_rangemax;

	len = int_index(tlv[1]);
	if (len > MAX_TLV_RANGE_SIZE)
		return -EINVAL;
	if (snd_tlv_get_dB_range(tlv, rangemin, rangemax, &dbmin, &dbmax))
		return -EINVAL;
	if (db_gain <= dbmin) {
		*value = rangemin;
		return 0;
	} else if (db_gain >= dbmax) {
		*value = rangemax;
		return 0;
	}
	pos = 2;
	prev_rangemax = 0;
	while (pos + 4 <= len) {
		rangemin = (int)tlv[pos++];
		rangemax = (int)tlv[pos++];
		if (!snd_tlv_get_dB_range(tlv + pos, rangemin, rangemax,
					  &dbmin, &dbmax) &&
		    db_gain >= dbmin && db_gain <= dbmax)
			return snd_tlv_convert_from_dB(tlv + pos,
						       rangemin, rangemax,
						       db_gain, value, xdir);
		else if (db_gain < dbmin) {
			*value = xdir ? rangemin : prev_rangemax;
			return 0;
		}
		prev_rangemax = rangemax;
		pos++;
		pos += int_index(tlv[pos]) + 1;
	}
	return -EINVAL;
}

static void _tlv_from_dB_minmax(long rangemin, long rangemax,
				long db_gain, long *value, int xdir,
				int min, int max, int have_mute)
{
	if (db_gain <= min) {
		if (db_gain > SND_CTL_TLV_DB_GAIN_MUTE && xdir > 0 && have_mute)
			*value = rangemin + 1;
		else
			*value = rangemin;
	} else if (db_gain >= max)
		*value = rangemax;
	else {
		long v = (db_gain - min) * (rangemax - rangemin);
		if (xdir > 0)
			v += (max - min) - 1;
		v = v / (max - min) + rangemin;
		*value = v;
	}
}

static int tlv_from_dB_scale(unsigned int *tlv, long rangemin, long rangemax,
			     long db_gain, long *value, int xdir)
{
	int min, step, max, have_mute;

	min = tlv[2];
	step = (tlv[3] & 0xffff);
	max = min + (int)(step * (rangemax - rangemin));
	have_mute = (tlv[3] & 0x10000);
	_tlv_from_dB_minmax(rangemin, rangemax, db_gain, value, xdir,
			    min, max, have_mute);
	return 0;
}

static int tlv_from_dB_minmax(unsigned int *tlv, long rangemin, long rangemax,
			      long db_gain, long *value, int xdir)
{
	int min, max, have_mute;

	min = tlv[2];
	max = tlv[3];
	have_mute = tlv[0] == SND_CTL_TLVT_DB_MINMAX_MUTE;
	_tlv_from_dB_minmax(rangemin, rangemax, db_gain, value, xdir,
			    min, max, have_mute);
	return 0;
}

#if SALSA_SUPPORT_FLOAT
static int tlv_from_dB_linear(unsigned int *tlv, long rangemin, long rangemax,
			      long db_gain, long *value, int xdir)
{
	int min, max;
	
	min = tlv[2];
	max = tlv[3];
	if (db_gain <= min)
		*value = rangemin;
	else if (db_gain >= max)
		*value = rangemax;
	else {
		double vmin, vmax, v;
		vmin = (min <= SND_CTL_TLV_DB_GAIN_MUTE) ? 0.0 :
			pow(10.0,  (double)min / 2000.0);
		vmax = !max ? 1.0 : pow(10.0,  (double)max / 2000.0);
		v = pow(10.0, (double)db_gain / 2000.0);
		v = (v - vmin) * (rangemax - rangemin) / (vmax - vmin);
		if (xdir > 0)
			v = ceil(v);
		*value = (long)v + rangemin;
	}
	return 0;
}
#else
static inline int tlv_from_dB_linear(unsigned int *tlv, long rangemin,
				     long rangemax, long db_gain,
				     long *value, int xdir)
{
	return -EINVAL;
}
#endif

int snd_tlv_convert_from_dB(unsigned int *tlv, long rangemin, long rangemax,
			    long db_gain, long *value, int xdir)
{
	switch (tlv[0]) {
	case SND_CTL_TLVT_DB_RANGE:
		return tlv_from_dB_range(tlv, rangemin, rangemax,
					 db_gain, value, xdir);
	case SND_CTL_TLVT_DB_SCALE:
		return tlv_from_dB_scale(tlv, rangemin, rangemax,
					 db_gain, value, xdir);
	case SND_CTL_TLVT_DB_MINMAX:
	case SND_CTL_TLVT_DB_MINMAX_MUTE:
		return tlv_from_dB_minmax(tlv, rangemin, rangemax,
					 db_gain, value, xdir);
	case SND_CTL_TLVT_DB_LINEAR:
		return tlv_from_dB_linear(tlv, rangemin, rangemax,
					  db_gain, value, xdir);
	}
	return -EINVAL;
}

#endif /* TLV */
