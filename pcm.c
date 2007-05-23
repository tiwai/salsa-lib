/*
 *  PCM Interface
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
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/poll.h>

static int get_device(const char *name, int *cardp, int *devp, int *subdevp)
{
	*cardp = 0;
	*devp = 0;
	*subdevp = -1;
	if (!strcmp(name, "hw") || !strcmp(name, "default"))
		return 0;
	if (sscanf(name, "default:%d", cardp) > 0)
		return 0;
	if (sscanf(name, "hw:%d,%d,%d", cardp, devp, subdevp) > 0)
		return 0;
	return -EINVAL;
}

int snd_pcm_open(snd_pcm_t **pcmp, const char *name, 
		 snd_pcm_stream_t stream, int mode)
{
	char filename[32];
	int card, dev, subdev;
	snd_ctl_t *ctl = NULL;
	int fd = -1, err, counts = 0;

	err = get_device(name, &card, &dev, &subdev);
	if (err < 0)
		return err;

	if (subdev >= 0) {
		char ctlfile[8];
		sprintf(ctlfile, "hw:%d", card);
		err = snd_ctl_open(&ctl, ctlfile, 0);
		if (err < 0)
			return err;
	}

	snprintf(filename, sizeof(filename), "%s/pcmC%dD%d%c",
		 DEVPATH, card, dev,
		 (stream == SND_PCM_STREAM_PLAYBACK ? 'p' : 'c'));
	fmode = O_RDWR | SND_PCM_NONBLOCK;
	if (mode & SND_PCM_ASYNC)
		fmode |= O_ASYNC;

 again:
	fd = open(filename, fmode);
	if (fd < 0) {
		err = -errno;
		goto error;
	}

	if (subdev >= 0) {
		memset(&info, 0, sizeof(info));
		if (ioctl(fd, SNDRV_PCM_IOCTL_INFO, &info) < 0) {
			err = -errno;
			goto error;
		}
		if (info.subdevice != subdev) {
			close(fd);
			fd = -1;
			if (counts++ > 3) {
				err = -EAGAIN;
				goto error;
			}
			goto again;
		}
		snd_ctl_close(ctl);
	}

	if (!(mode & SND_PCM_NONBLOCK)) {
		fmode &= ~O_NONBLOCK;
		fcntl(fd, F_SETFD, fmode);
	}

	if (ioctl(fd, SNDRV_PCM_IOCTL_PVERSION, &ver) < 0) {
		err = -errno;
		goto error;
	}
	if (SNDRV_PROTOCOL_INCOMPATIBLE(ver, SNDRV_PCM_VERSION_MAX)) {
		err = -SND_ERROR_INCOMPATIBLE_VERSION;
		goto error;
	}

	pcm = create_pcm(name, fd);
	if (!pcm) {
		err = -ENOMEM;
		goto error;
	}

	pcm->pollfd.fd = fd;
	pcm->pollfd.events =
		(stream == SND_PCM_STREAM_PLAYBACK ? POLLOUT : POLLIN)
		| POLLERR | POLLNVAL;

	return 0;

 error:
	if (ctl)
		snd_ctl_close(ctl);
	if (fd >= 0)
		close(fd);
	return err;
}

int snd_pcm_close(snd_pcm_t *pcm)
{
	if (pcm->setup) {
		snd_pcm_drop(pcm);
		snd_pcm_hw_free(pcm);
	}
	if (pcm->mmap_channels)
		snd_pcm_munmap(pcm);
#if 0 // ASYNC
	while (!list_empty(&pcm->async_handlers)) {
		snd_async_handler_t *h = list_entry(pcm->async_handlers.next, snd_async_handler_t, hlist);
		snd_async_del_handler(h);
	}
#endif
	close(pcm->fd);
	snd_pcm_free(pcm);
	return 0;
}	

int snd_pcm_nonblock(snd_pcm_t *pcm, int nonblock)
{
	int flags = fcntl(pcm->fd, F_GETFD);

	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	fcntl(fd, F_SETFD, flags);
	return 0;
}

static int snd_pcm_async(snd_pcm_t *pcm, int sig, pid_t pid)
{
	if (sig == 0)
		sig = SIGIO;
	if (pid == 0)
		pid = getpid();
	return pcm->ops->async(pcm->op_arg, sig, pid);
}

static int snd_pcm_sw_params_default(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
	params->tstamp_mode = SND_PCM_TSTAMP_NONE;
	params->period_step = 1;
	params->sleep_min = 0;
	params->avail_min = pcm->period_size;
	params->xfer_align = pcm->period_size;
	params->start_threshold = 1;
	params->stop_threshold = pcm->buffer_size;
	params->silence_threshold = 0;
	params->silence_size = 0;
	params->boundary = pcm->buffer_size;
	while (params->boundary * 2 <= LONG_MAX - pcm->buffer_size)
		params->boundary *= 2;
	return 0;
}

static int snd_pcm_hw_refine(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_REFINE, params) < 0) {
		err = -errno;
		return err;
	}
	return 0;
}

/*
 */
static inline int hw_is_mask(snd_pcm_hw_param_t var)
{
	return var >= SND_PCM_HW_PARAM_FIRST_MASK &&
		var <= SND_PCM_HW_PARAM_LAST_MASK;
}

static inline int hw_is_interval(snd_pcm_hw_param_t var)
{
	return var >= SND_PCM_HW_PARAM_FIRST_INTERVAL &&
		var <= SND_PCM_HW_PARAM_LAST_INTERVAL;
}

static void _snd_pcm_hw_param_any(snd_pcm_hw_params_t *params, snd_pcm_hw_param_t var)
{
	if (hw_is_mask(var)) {
		snd_mask_any(hw_param_mask(params, var));
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	} else {
		snd_interval_any(hw_param_interval(params, var));
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
}

static void _snd_pcm_hw_params_any(snd_pcm_hw_params_t *params)
{
	unsigned int k;
	memset(params, 0, sizeof(*params));
	for (k = SND_PCM_HW_PARAM_FIRST_MASK; k <= SND_PCM_HW_PARAM_LAST_MASK; k++)
		_snd_pcm_hw_param_any(params, k);
	for (k = SND_PCM_HW_PARAM_FIRST_INTERVAL; k <= SND_PCM_HW_PARAM_LAST_INTERVAL; k++)
		_snd_pcm_hw_param_any(params, k);
	params->rmask = ~0U;
	params->cmask = 0;
	params->info = ~0U;
}

static int _snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	int err;
	snd_pcm_sw_params_t sw;

	err = snd_pcm_hw_refine(pcm, params);
	if (err < 0)
		return err;
	snd_pcm_hw_params_choose(pcm, params);
	snd_pcm_hw_free(pcm);
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, params) < 0)
		return -errno;

	pcm->setup = 1;
	*pcm->hw_params = *params;
	
	snd_pcm_hw_params_get_access(params, &pcm->access);
	snd_pcm_hw_params_get_format(params, &pcm->format);
	snd_pcm_hw_params_get_subformat(params, &pcm->subformat);
	snd_pcm_hw_params_get_channels(params, &pcm->channels);
	snd_pcm_hw_params_get_rate(params, &pcm->rate, 0);
	snd_pcm_hw_params_get_period_time(params, &pcm->period_time, 0);
	snd_pcm_hw_params_get_period_size(params, &pcm->period_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &pcm->buffer_size);
	snd_pcm_hw_params_get_tick_time(params, &pcm->tick_time, 0);
	pcm->sample_bits = snd_pcm_format_physical_width(pcm->format);
	pcm->frame_bits = pcm->sample_bits * pcm->channels;

	/* Default sw params */
	memset(&sw, 0, sizeof(sw));
	snd_pcm_sw_params_default(pcm, &sw);
	snd_pcm_sw_params(pcm, &sw);
	return 0;
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	int err;
	err = _snd_pcm_hw_params(pcm, params);
	if (err < 0)
		return err;
	err = snd_pcm_prepare(pcm);
	return err;
}

int snd_pcm_hw_free(snd_pcm_t *pcm)
{
	int err;
	if (!pcm->setup)
		return 0;
	if (pcm->mmap_channels) {
		err = snd_pcm_munmap(pcm);
		if (err < 0)
			return err;
		pcm->mmap_channels = 0;
	}
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_FREE) < 0)
		return -errno;
	pcm->setup = 0;
	return 0;
}

static int correct_pcm_error(snd_pcm_t *pcm, int err)
{
	switch (snd_pcm_state(pcm)) {
	case SND_PCM_STATE_XRUN:
		return -EPIPE;
	case SND_PCM_STATE_SUSPENDED:
		return -ESTRPIPE;
	case SND_PCM_STATE_DISCONNECTED:
		return -ENODEV;
	default:
		return err;
	}
}

static inline int snd_pcm_check_error(snd_pcm_t *pcm, int err)
{
	if (err == -EINTR)
		return correct_pcm_error(pcm, err);
	return err;
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer,
				 snd_pcm_uframes_t size)
{
	struct sndrv_xferi xferi;

	xferi.buf = (char*) buffer;
	xferi.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &xferi) < 0)
		return snd_pcm_check_error(pcm, -errno);
	return xferi.result;
}

snd_pcm_sframes_t snd_pcm_writen(snd_pcm_t *pcm, void **bufs,
				 snd_pcm_uframes_t size)
{
	struct sndrv_xfern xfern;

	xfern.bufs = bufs;
	xfern.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEN_FRAMES, &xfern) < 0)
		return snd_pcm_check_error(pcm, -errno);
	return xfern.result;
}

snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer,
				snd_pcm_uframes_t size)
{
	struct sndrv_xferi xferi;

	xferi.buf = buffer;
	xferi.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &xferi) < 0)
		return snd_pcm_check_error(pcm, -errno);
	return xferi.result;
}

snd_pcm_sframes_t snd_pcm_readn(snd_pcm_t *pcm, void **bufs,
				snd_pcm_uframes_t size)
{
	struct sndrv_xfern xfern;

	xfern.bufs = bufs;
	xfern.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READN_FRAMES, &xfern) < 0)
		return snd_pcm_check_error(pcm, -errno);
	return xfern.result;
}

#define PCMTYPE(v) [SND_PCM_TYPE_##v] = #v
#define STATE(v) [SND_PCM_STATE_##v] = #v
#define STREAM(v) [SND_PCM_STREAM_##v] = #v
#define READY(v) [SND_PCM_READY_##v] = #v
#define XRUN(v) [SND_PCM_XRUN_##v] = #v
#define SILENCE(v) [SND_PCM_SILENCE_##v] = #v
#define TSTAMP(v) [SND_PCM_TSTAMP_##v] = #v
#define ACCESS(v) [SND_PCM_ACCESS_##v] = #v
#define START(v) [SND_PCM_START_##v] = #v
#define HW_PARAM(v) [SND_PCM_HW_PARAM_##v] = #v
#define SW_PARAM(v) [SND_PCM_SW_PARAM_##v] = #v
#define FORMAT(v) [SND_PCM_FORMAT_##v] = #v
#define SUBFORMAT(v) [SND_PCM_SUBFORMAT_##v] = #v 

#define FORMATD(v, d) [SND_PCM_FORMAT_##v] = d
#define SUBFORMATD(v, d) [SND_PCM_SUBFORMAT_##v] = d 


const char *_snd_pcm_stream_names[] = {
	STREAM(PLAYBACK),
	STREAM(CAPTURE),
};

const char *_snd_pcm_state_names[] = {
	STATE(OPEN),
	STATE(SETUP),
	STATE(PREPARED),
	STATE(RUNNING),
	STATE(XRUN),
	STATE(DRAINING),
	STATE(PAUSED),
	STATE(SUSPENDED),
	STATE(DISCONNECTED),
};

const char *_snd_pcm_access_names[] = {
	ACCESS(MMAP_INTERLEAVED), 
	ACCESS(MMAP_NONINTERLEAVED),
	ACCESS(MMAP_COMPLEX),
	ACCESS(RW_INTERLEAVED),
	ACCESS(RW_NONINTERLEAVED),
};

const char *_snd_pcm_format_names[] = {
	FORMAT(S8),
	FORMAT(U8),
	FORMAT(S16_LE),
	FORMAT(S16_BE),
	FORMAT(U16_LE),
	FORMAT(U16_BE),
	FORMAT(S24_LE),
	FORMAT(S24_BE),
	FORMAT(U24_LE),
	FORMAT(U24_BE),
	FORMAT(S32_LE),
	FORMAT(S32_BE),
	FORMAT(U32_LE),
	FORMAT(U32_BE),
	FORMAT(FLOAT_LE),
	FORMAT(FLOAT_BE),
	FORMAT(FLOAT64_LE),
	FORMAT(FLOAT64_BE),
	FORMAT(IEC958_SUBFRAME_LE),
	FORMAT(IEC958_SUBFRAME_BE),
	FORMAT(MU_LAW),
	FORMAT(A_LAW),
	FORMAT(IMA_ADPCM),
	FORMAT(MPEG),
	FORMAT(GSM),
	FORMAT(SPECIAL),
	FORMAT(S24_3LE),
	FORMAT(S24_3BE),
	FORMAT(U24_3LE),
	FORMAT(U24_3BE),
	FORMAT(S20_3LE),
	FORMAT(S20_3BE),
	FORMAT(U20_3LE),
	FORMAT(U20_3BE),
	FORMAT(S18_3LE),
	FORMAT(S18_3BE),
	FORMAT(U18_3LE),
	FORMAT(U18_3BE),
};

static const char *_snd_pcm_format_aliases[SND_PCM_FORMAT_LAST+1] = {
	FORMAT(S16),
	FORMAT(U16),
	FORMAT(S24),
	FORMAT(U24),
	FORMAT(S32),
	FORMAT(U32),
	FORMAT(FLOAT),
	FORMAT(FLOAT64),
	FORMAT(IEC958_SUBFRAME),
};

const char *_snd_pcm_format_descriptions[] = {
	FORMATD(S8, "Signed 8 bit"), 
	FORMATD(U8, "Unsigned 8 bit"),
	FORMATD(S16_LE, "Signed 16 bit Little Endian"),
	FORMATD(S16_BE, "Signed 16 bit Big Endian"),
	FORMATD(U16_LE, "Unsigned 16 bit Little Endian"),
	FORMATD(U16_BE, "Unsigned 16 bit Big Endian"),
	FORMATD(S24_LE, "Signed 24 bit Little Endian"),
	FORMATD(S24_BE, "Signed 24 bit Big Endian"),
	FORMATD(U24_LE, "Unsigned 24 bit Little Endian"),
	FORMATD(U24_BE, "Unsigned 24 bit Big Endian"),
	FORMATD(S32_LE, "Signed 32 bit Little Endian"),
	FORMATD(S32_BE, "Signed 32 bit Big Endian"),
	FORMATD(U32_LE, "Unsigned 32 bit Little Endian"),
	FORMATD(U32_BE, "Unsigned 32 bit Big Endian"),
	FORMATD(FLOAT_LE, "Float 32 bit Little Endian"),
	FORMATD(FLOAT_BE, "Float 32 bit Big Endian"),
	FORMATD(FLOAT64_LE, "Float 64 bit Little Endian"),
	FORMATD(FLOAT64_BE, "Float 64 bit Big Endian"),
	FORMATD(IEC958_SUBFRAME_LE, "IEC-958 Little Endian"),
	FORMATD(IEC958_SUBFRAME_BE, "IEC-958 Big Endian"),
	FORMATD(MU_LAW, "Mu-Law"),
	FORMATD(A_LAW, "A-Law"),
	FORMATD(IMA_ADPCM, "Ima-ADPCM"),
	FORMATD(MPEG, "MPEG"),
	FORMATD(GSM, "GSM"),
	FORMATD(SPECIAL, "Special"),
	FORMATD(S24_3LE, "Signed 24 bit Little Endian in 3bytes"),
	FORMATD(S24_3BE, "Signed 24 bit Big Endian in 3bytes"),
	FORMATD(U24_3LE, "Unsigned 24 bit Little Endian in 3bytes"),
	FORMATD(U24_3BE, "Unsigned 24 bit Big Endian in 3bytes"),
	FORMATD(S20_3LE, "Signed 20 bit Little Endian in 3bytes"),
	FORMATD(S20_3BE, "Signed 20 bit Big Endian in 3bytes"),
	FORMATD(U20_3LE, "Unsigned 20 bit Little Endian in 3bytes"),
	FORMATD(U20_3BE, "Unsigned 20 bit Big Endian in 3bytes"),
	FORMATD(S18_3LE, "Signed 18 bit Little Endian in 3bytes"),
	FORMATD(S18_3BE, "Signed 18 bit Big Endian in 3bytes"),
	FORMATD(U18_3LE, "Unsigned 18 bit Little Endian in 3bytes"),
	FORMATD(U18_3BE, "Unsigned 18 bit Big Endian in 3bytes"),
};

const char *_snd_pcm_type_names[] = {
	PCMTYPE(HW), 
	PCMTYPE(HOOKS), 
	PCMTYPE(MULTI), 
	PCMTYPE(FILE), 
	PCMTYPE(NULL), 
	PCMTYPE(SHM), 
	PCMTYPE(INET), 
	PCMTYPE(COPY), 
	PCMTYPE(LINEAR), 
	PCMTYPE(ALAW), 
	PCMTYPE(MULAW), 
	PCMTYPE(ADPCM), 
	PCMTYPE(RATE), 
	PCMTYPE(ROUTE), 
	PCMTYPE(PLUG), 
	PCMTYPE(SHARE), 
	PCMTYPE(METER), 
	PCMTYPE(MIX), 
	PCMTYPE(DROUTE), 
	PCMTYPE(LBSERVER), 
	PCMTYPE(LINEAR_FLOAT), 
	PCMTYPE(LADSPA), 
	PCMTYPE(DMIX), 
	PCMTYPE(JACK),
        PCMTYPE(DSNOOP),
        PCMTYPE(IEC958),
	PCMTYPE(SOFTVOL),
        PCMTYPE(IOPLUG),
        PCMTYPE(EXTPLUG),
};

const char *_snd_pcm_subformat_names[] = {
	SUBFORMAT(STD), 
};

const char *_snd_pcm_subformat_descriptions[] = {
	SUBFORMATD(STD, "Standard"), 
};

const char *_snd_pcm_tstamp_mode_names[] = {
	TSTAMP(NONE),
	TSTAMP(MMAP),
};

snd_pcm_format_t snd_pcm_format_value(const char* name)
{
	snd_pcm_format_t format;
	for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
		if (snd_pcm_format_names[format] &&
		    strcasecmp(name, snd_pcm_format_names[format]) == 0) {
			return format;
		}
		if (snd_pcm_format_aliases[format] &&
		    strcasecmp(name, snd_pcm_format_aliases[format]) == 0) {
			return format;
		}
	}
	for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
		if (snd_pcm_format_descriptions[format] &&
		    strcasecmp(name, snd_pcm_format_descriptions[format]) == 0) {
			return format;
		}
	}
	return SND_PCM_FORMAT_UNKNOWN;
}


int snd_pcm_dump_hw_setup(snd_pcm_t *pcm, snd_output_t *out)
{
	snd_output_printf(out, "  stream       : %s\n",
			  snd_pcm_stream_name(pcm->stream));
	snd_output_printf(out, "  access       : %s\n",
			  snd_pcm_access_name(pcm->access));
	snd_output_printf(out, "  format       : %s\n",
			  snd_pcm_format_name(pcm->format));
	snd_output_printf(out, "  subformat    : %s\n",
			  snd_pcm_subformat_name(pcm->subformat));
	snd_output_printf(out, "  channels     : %u\n", pcm->channels);
	snd_output_printf(out, "  rate         : %u\n", pcm->rate);
	snd_output_printf(out, "  exact rate   : %g (%u/%u)\n",
			  (pcm->hw_params.rate_den ?
			   ((double) pcm->hw_params.rate_num / pcm->hw_params.rate_den) : 0.0),
			  pcm->hw_params.rate_num, pcm->hw_params.rate_den);
	snd_output_printf(out, "  msbits       : %u\n", pcm->hw_params.msbits);
	snd_output_printf(out, "  buffer_size  : %lu\n", pcm->buffer_size);
	snd_output_printf(out, "  period_size  : %lu\n", pcm->period_size);
	snd_output_printf(out, "  period_time  : %u\n", pcm->period_time);
	snd_output_printf(out, "  tick_time    : %u\n", pcm->tick_time);
	return 0;
}

int snd_pcm_dump_sw_setup(snd_pcm_t *pcm, snd_output_t *out)
{
	snd_output_printf(out, "  tstamp_mode  : %s\n",
			  snd_pcm_tstamp_mode_name(pcm->sw_params.tstamp_mode));
	snd_output_printf(out, "  period_step  : %d\n",
			  pcm->sw_params.period_step);
	snd_output_printf(out, "  sleep_min    : %d\n",
			  pcm->sw_params.sleep_min);
	snd_output_printf(out, "  avail_min    : %ld\n",
			  pcm->sw_params.avail_min);
	snd_output_printf(out, "  xfer_align   : %ld\n",
			  pcm->sw_params.xfer_align);
	snd_output_printf(out, "  start_threshold  : %ld\n",
			  pcm->sw_params.start_threshold);
	snd_output_printf(out, "  stop_threshold   : %ld\n",
			  pcm->sw_params.stop_threshold);
	snd_output_printf(out, "  silence_threshold: %ld\n",
			  pcm->sw_params.silence_threshold);
	snd_output_printf(out, "  silence_size : %ld\n",
			  pcm->sw_params.silence_size);
	snd_output_printf(out, "  boundary     : %ld\n",
			  pcm->sw_params.boundary);
	return 0;
}

int snd_pcm_dump_setup(snd_pcm_t *pcm, snd_output_t *out)
{
	snd_pcm_dump_hw_setup(pcm, out);
	snd_pcm_dump_sw_setup(pcm, out);
	return 0;
}

int snd_pcm_status_dump(snd_pcm_status_t *status, snd_output_t *out)
{
	snd_output_printf(out, "  state       : %s\n",
			  snd_pcm_state_name((snd_pcm_state_t) status->state));
	snd_output_printf(out, "  trigger_time: %ld.%06ld\n",
		status->trigger_tstamp.tv_sec, status->trigger_tstamp.tv_nsec);
	snd_output_printf(out, "  tstamp      : %ld.%06ld\n",
		status->tstamp.tv_sec, status->tstamp.tv_nsec);
	snd_output_printf(out, "  delay       : %ld\n", (long)status->delay);
	snd_output_printf(out, "  avail       : %ld\n", (long)status->avail);
	snd_output_printf(out, "  avail_max   : %ld\n", (long)status->avail_max);
	return 0;
}

int snd_pcm_dump(snd_pcm_t *pcm, snd_output_t *out)
{
	char *name;
	int err = snd_card_get_name(pcm->card, &name);
	if (err < 0)
		return err;
	snd_output_printf(out,
			  "Hardware PCM card %d '%s' device %d subdevice %d\n",
			  pcm->card, name, pcm->device, pcm->subdevice);
	free(name);
	return 0;
}

#if 0 // ASYNC
int snd_async_add_pcm_handler(snd_async_handler_t **handler, snd_pcm_t *pcm, 
			      snd_async_callback_t callback, void *private_data)
{
	int err;
	int was_empty;
	snd_async_handler_t *h;
	err = snd_async_add_handler(&h, _snd_pcm_async_descriptor(pcm),
				    callback, private_data);
	if (err < 0)
		return err;
	h->type = SND_ASYNC_HANDLER_PCM;
	h->u.pcm = pcm;
	was_empty = list_empty(&pcm->async_handlers);
	list_add_tail(&h->hlist, &pcm->async_handlers);
	if (was_empty) {
		err = snd_pcm_async(pcm, snd_async_handler_get_signo(h), getpid());
		if (err < 0) {
			snd_async_del_handler(h);
			return err;
		}
	}
	*handler = h;
	return 0;
}

snd_pcm_t *snd_async_handler_get_pcm(snd_async_handler_t *handler)
{
	if (handler->type != SND_ASYNC_HANDLER_PCM) {
		SNDMSG("invalid handler type %d", handler->type);
		return NULL;
	}
	return handler->u.pcm;
}
#endif // ASYNC

int snd_pcm_wait(snd_pcm_t *pcm, int timeout)
{
	struct pollfd pfd;
	int i, npfds, err, err_poll;
	
	if (snd_pcm_mmap_avail(pcm) >= pcm->sw_params.avail_min)
		return correct_pcm_error(pcm, 1);

	pfd = pcm->pollfd;
	for (;;) {
		err = poll(&pfd, 1, timeout);
		if (err < 0) {
			if (errno == EINTR)
				continue;
			return -errno;
                }
		if (!err)
			return 0;
		if (pfd.revents & (POLLERR | POLLNVAL))
			return correct_pcm_error(pcm, -EIO);
		if (pfd.revents & (POLLIN | POLLOUT))
			return 1;
	}
}

snd_pcm_sframes_t snd_pcm_avail_update(snd_pcm_t *pcm)
{
	if (pcm->stream == SND_PCM_STREAM_PLAYBACK) {
		avail = snd_pcm_mmap_playback_avail(pcm);
	} else {
		avail = snd_pcm_mmap_capture_avail(pcm);
	}
	switch (FAST_PCM_STATE(hw)) {
	case SNDRV_PCM_STATE_RUNNING:
		if (avail >= pcm->stop_threshold) {
			if (ioctl(hw->fd, SNDRV_PCM_IOCTL_XRUN) < 0)
				return -errno;
			/* everything is ok, state == SND_PCM_STATE_XRUN at the moment */
			return -EPIPE;
		}
		break;
	case SNDRV_PCM_STATE_XRUN:
		return -EPIPE;
	default:
		break;
	}
	return avail;
}

int snd_pcm_area_silence(const snd_pcm_channel_area_t *dst_area,
			 snd_pcm_uframes_t dst_offset,
			 unsigned int samples, snd_pcm_format_t format)
{
	char *dst;
	unsigned int dst_step;
	int width;
	u_int64_t silence;
	if (!dst_area->addr)
		return 0;
	dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
	width = snd_pcm_format_physical_width(format);
	silence = snd_pcm_format_silence_64(format);
	if (dst_area->step == (unsigned int) width) {
		unsigned int dwords = samples * width / 64;
		u_int64_t *dstp = (u_int64_t *)dst;
		samples -= dwords * 64 / width;
		while (dwords-- > 0)
			*dstp++ = silence;
		if (samples == 0)
			return 0;
	}
	dst_step = dst_area->step / 8;
	switch (width) {
	case 4: {
		u_int8_t s0 = silence & 0xf0;
		u_int8_t s1 = silence & 0x0f;
		int dstbit = dst_area->first % 8;
		int dstbit_step = dst_area->step % 8;
		while (samples-- > 0) {
			if (dstbit) {
				*dst &= 0xf0;
				*dst |= s1;
			} else {
				*dst &= 0x0f;
				*dst |= s0;
			}
			dst += dst_step;
			dstbit += dstbit_step;
			if (dstbit == 8) {
				dst++;
				dstbit = 0;
			}
		}
		break;
	}
	case 8: {
		u_int8_t sil = silence;
		while (samples-- > 0) {
			*dst = sil;
			dst += dst_step;
		}
		break;
	}
	case 16: {
		u_int16_t sil = silence;
		while (samples-- > 0) {
			*(u_int16_t*)dst = sil;
			dst += dst_step;
		}
		break;
	}
	case 24:
#ifdef SNDRV_LITTLE_ENDIAN
		*(dst + 0) = silence >> 0;
		*(dst + 1) = silence >> 8;
		*(dst + 2) = silence >> 16;
#else
		*(dst + 2) = silence >> 0;
		*(dst + 1) = silence >> 8;
		*(dst + 0) = silence >> 16;
#endif
		break;
	case 32: {
		u_int32_t sil = silence;
		while (samples-- > 0) {
			*(u_int32_t*)dst = sil;
			dst += dst_step;
		}
		break;
	}
	case 64: {
		while (samples-- > 0) {
			*(u_int64_t*)dst = silence;
			dst += dst_step;
		}
		break;
	}
	default:
		SNDMSG("invalid format width %d", width);
		return -EINVAL;
	}
	return 0;
}

int snd_pcm_areas_silence(const snd_pcm_channel_area_t *dst_areas,
			  snd_pcm_uframes_t dst_offset,
			  unsigned int channels, snd_pcm_uframes_t frames,
			  snd_pcm_format_t format)
{
	int width = snd_pcm_format_physical_width(format);
	while (channels > 0) {
		void *addr = dst_areas->addr;
		unsigned int step = dst_areas->step;
		const snd_pcm_channel_area_t *begin = dst_areas;
		int channels1 = channels;
		unsigned int chns = 0;
		int err;
		while (1) {
			channels1--;
			chns++;
			dst_areas++;
			if (channels1 == 0 ||
			    dst_areas->addr != addr ||
			    dst_areas->step != step ||
			    dst_areas->first != dst_areas[-1].first + width)
				break;
		}
		if (chns > 1 && chns * width == step) {
			/* Collapse the areas */
			snd_pcm_channel_area_t d;
			d.addr = begin->addr;
			d.first = begin->first;
			d.step = width;
			err = snd_pcm_area_silence(&d, dst_offset * chns, frames * chns, format);
			channels -= chns;
		} else {
			err = snd_pcm_area_silence(begin, dst_offset, frames, format);
			dst_areas = begin + 1;
			channels--;
		}
		if (err < 0)
			return err;
	}
	return 0;
}


int snd_pcm_area_copy(const snd_pcm_channel_area_t *dst_area,
		      snd_pcm_uframes_t dst_offset,
		      const snd_pcm_channel_area_t *src_area,
		      snd_pcm_uframes_t src_offset,
		      unsigned int samples, snd_pcm_format_t format)
{
	const char *src;
	char *dst;
	int width;
	int src_step, dst_step;
	if (dst_area == src_area && dst_offset == src_offset)
		return 0;
	if (!src_area->addr)
		return snd_pcm_area_silence(dst_area, dst_offset, samples, format);
	src = snd_pcm_channel_area_addr(src_area, src_offset);
	if (!dst_area->addr)
		return 0;
	dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
	width = snd_pcm_format_physical_width(format);
	if (src_area->step == (unsigned int) width &&
	    dst_area->step == (unsigned int) width) {
		size_t bytes = samples * width / 8;
		samples -= bytes * 8 / width;
		memcpy(dst, src, bytes);
		if (samples == 0)
			return 0;
	}
	src_step = src_area->step / 8;
	dst_step = dst_area->step / 8;
	switch (width) {
	case 4: {
		int srcbit = src_area->first % 8;
		int srcbit_step = src_area->step % 8;
		int dstbit = dst_area->first % 8;
		int dstbit_step = dst_area->step % 8;
		while (samples-- > 0) {
			unsigned char srcval;
			if (srcbit)
				srcval = *src & 0x0f;
			else
				srcval = *src & 0xf0;
			if (dstbit)
				*dst &= 0xf0;
			else
				*dst &= 0x0f;
			*dst |= srcval;
			src += src_step;
			srcbit += srcbit_step;
			if (srcbit == 8) {
				src++;
				srcbit = 0;
			}
			dst += dst_step;
			dstbit += dstbit_step;
			if (dstbit == 8) {
				dst++;
				dstbit = 0;
			}
		}
		break;
	}
	case 8: {
		while (samples-- > 0) {
			*dst = *src;
			src += src_step;
			dst += dst_step;
		}
		break;
	}
	case 16: {
		while (samples-- > 0) {
			*(u_int16_t*)dst = *(const u_int16_t*)src;
			src += src_step;
			dst += dst_step;
		}
		break;
	}
	case 24:
		while (samples-- > 0) {
			*(dst + 0) = *(src + 0);
			*(dst + 1) = *(src + 1);
			*(dst + 2) = *(src + 2);
			src += src_step;
			dst += dst_step;
		}
		break;
	case 32: {
		while (samples-- > 0) {
			*(u_int32_t*)dst = *(const u_int32_t*)src;
			src += src_step;
			dst += dst_step;
		}
		break;
	}
	case 64: {
		while (samples-- > 0) {
			*(u_int64_t*)dst = *(const u_int64_t*)src;
			src += src_step;
			dst += dst_step;
		}
		break;
	}
	default:
		SNDMSG("invalid format width %d", width);
		return -EINVAL;
	}
	return 0;
}

int snd_pcm_areas_copy(const snd_pcm_channel_area_t *dst_areas,
		       snd_pcm_uframes_t dst_offset,
		       const snd_pcm_channel_area_t *src_areas,
		       snd_pcm_uframes_t src_offset,
		       unsigned int channels, snd_pcm_uframes_t frames,
		       snd_pcm_format_t format)
{
	int width = snd_pcm_format_physical_width(format);
	if (! channels) {
		SNDMSG("invalid channels %d", channels);
		return -EINVAL;
	}
	if (! frames) {
		SNDMSG("invalid frames %ld", frames);
		return -EINVAL;
	}
	while (channels > 0) {
		unsigned int step = src_areas->step;
		void *src_addr = src_areas->addr;
		const snd_pcm_channel_area_t *src_start = src_areas;
		void *dst_addr = dst_areas->addr;
		const snd_pcm_channel_area_t *dst_start = dst_areas;
		int channels1 = channels;
		unsigned int chns = 0;
		while (dst_areas->step == step) {
			channels1--;
			chns++;
			src_areas++;
			dst_areas++;
			if (channels1 == 0 ||
			    src_areas->step != step ||
			    src_areas->addr != src_addr ||
			    dst_areas->addr != dst_addr ||
			    src_areas->first != src_areas[-1].first + width ||
			    dst_areas->first != dst_areas[-1].first + width)
				break;
		}
		if (chns > 1 && chns * width == step) {
			/* Collapse the areas */
			snd_pcm_channel_area_t s, d;
			s.addr = src_start->addr;
			s.first = src_start->first;
			s.step = width;
			d.addr = dst_start->addr;
			d.first = dst_start->first;
			d.step = width;
			snd_pcm_area_copy(&d, dst_offset * chns,
					  &s, src_offset * chns, 
					  frames * chns, format);
			channels -= chns;
		} else {
			snd_pcm_area_copy(dst_start, dst_offset,
					  src_start, src_offset,
					  frames, format);
			src_areas = src_start + 1;
			dst_areas = dst_start + 1;
			channels--;
		}
	}
	return 0;
}

void snd_pcm_hw_param_dump(const snd_pcm_hw_params_t *params,
			   snd_pcm_hw_param_t var, snd_output_t *out)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *mask = hw_param_mask_c(params, var);
		if (snd_mask_empty(mask))
			snd_output_puts(out, " NONE");
		else if (snd_mask_full(mask))
			snd_output_puts(out, " ALL");
		else {
			unsigned int k;
			for (k = 0; k <= SND_MASK_MAX; ++k) {
				if (snd_mask_test(mask, k)) {
					const char *s;
					switch (var) {
					case SND_PCM_HW_PARAM_ACCESS:
						s = snd_pcm_access_name(k);
						break;
					case SND_PCM_HW_PARAM_FORMAT:
						s = snd_pcm_format_name(k);
						break;
					case SND_PCM_HW_PARAM_SUBFORMAT:
						s = snd_pcm_subformat_name(k);
						break;
					default:
						s = NULL;
					}
					if (s) {
						snd_output_putc(out, ' ');
						snd_output_puts(out, s);
					}
				}
			}
		}
		return;
	}
	if (hw_is_interval(var)) {
		snd_interval_print(hw_param_interval_c(params, var), out);
		return;
	}
}

static void dump_one_param(snd_pcm_hw_params_t *params, unsigned int k,
			   snd_output_t *out)
{
	snd_output_printf(out, "%s: ", snd_pcm_hw_param_name(k));
	snd_pcm_hw_param_dump(params, k, out);
	snd_output_putc(out, '\n');
}

int snd_pcm_hw_params_dump(snd_pcm_hw_params_t *params, snd_output_t *out)
{
	unsigned int k;
	for (k = SND_PCM_HW_PARAM_FIRST_MASK; k <= SND_PCM_HW_PARAM_LAST_MASK; k++)
		dump_one_param(params, k, out);
	for (k = SND_PCM_HW_PARAM_FIRST_INTERVAL; k <= SND_PCM_HW_PARAM_LAST_INTERVAL; k++)
		dump_one_param(params, k, out);
	return 0;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	_snd_pcm_hw_params_any(params);
	return snd_pcm_hw_refine(pcm, params);
}

int snd_pcm_mmap_begin(snd_pcm_t *pcm,
		       const snd_pcm_channel_area_t **areas,
		       snd_pcm_uframes_t *offset,
		       snd_pcm_uframes_t *frames)
{
	snd_pcm_uframes_t cont;
	snd_pcm_uframes_t f;
	snd_pcm_uframes_t avail;
	const snd_pcm_channel_area_t *xareas;
	assert(pcm && areas && offset && frames);
	xareas = snd_pcm_mmap_areas(pcm);
	if (xareas == NULL)
		return -EBADFD;
	*areas = xareas;
	*offset = *pcm->appl.ptr % pcm->buffer_size;
	avail = snd_pcm_mmap_avail(pcm);
	if (avail > pcm->buffer_size)
		avail = pcm->buffer_size;
	cont = pcm->buffer_size - *offset;
	f = *frames;
	if (f > avail)
		f = avail;
	if (f > cont)
		f = cont;
	*frames = f;
	return 0;
}

snd_pcm_sframes_t snd_pcm_mmap_commit(snd_pcm_t *pcm,
				      snd_pcm_uframes_t offset,
				      snd_pcm_uframes_t frames)
{
	return pcm->fast_ops->mmap_commit(pcm->fast_op_arg, offset, frames);
}


int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
        if (err > 0)
                err = -err;
        if (err == -EINTR)	/* nothing to do, continue */
                return 0;
        if (err == -EPIPE) {
                err = snd_pcm_prepare(pcm);
                if (err < 0)
                        return err;
                return 0;
        }
        if (err == -ESTRPIPE) {
                while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
                        /* wait until suspend flag is released */
                        poll(NULL, 0, 1000);
                if (err < 0) {
                        err = snd_pcm_prepare(pcm);
                        if (err < 0)
                                return err;
                }
                return 0;
        }
        return err;
}



int snd_pcm_hw_params_get_min_align(const snd_pcm_hw_params_t *params,
				    snd_pcm_uframes_t *val)
{
	unsigned int format, channels, fb, min_align;
	int err;

	err = snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_FORMAT, &format, NULL);
	if (err < 0)
		return err;
	err = snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_CHANNELS, &channels, NULL);
	if (err < 0)
		return err;
	// compute frame bits
	fb = snd_pcm_format_physical_width((snd_pcm_format_t)format) * channels;
        min_align = 1;
	while (fb % 8) {
		fb *= 2;
                min_align *= 2;
	}
	if (val)
		*val = min_align;
	return 0;
}


int snd_pcm_sw_params_dump(snd_pcm_sw_params_t *params, snd_output_t *out)
{
	snd_output_printf(out, "start_mode: %s\n",
			  snd_pcm_start_mode_name(snd_pcm_sw_params_get_start_mode(params)));
	snd_output_printf(out, "xrun_mode: %s\n",
			  snd_pcm_xrun_mode_name(snd_pcm_sw_params_get_xrun_mode(params)));
	snd_output_printf(out, "tstamp_mode: %s\n",
			  snd_pcm_tstamp_mode_name(params->tstamp_mode));
	snd_output_printf(out, "period_step: %u\n",
			  params->period_step);
	snd_output_printf(out, "sleep_min: %u\n",
			  params->sleep_min);
	snd_output_printf(out, "avail_min: %lu\n",
			  params->avail_min);
	snd_output_printf(out, "xfer_align: %lu\n",
			  params->xfer_align);
	snd_output_printf(out, "silence_threshold: %lu\n",
			  params->silence_threshold);
	snd_output_printf(out, "silence_size: %lu\n",
			  params->silence_size);
	snd_output_printf(out, "boundary: %lu\n",
			  params->boundary);
	return 0;
}

