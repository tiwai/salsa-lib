/*
 * HW/SW PARAMS
 */

/*
 * non-inlined mask operations
 */
#define MASK_SIZE (SND_MASK_MAX / 32)
#define MASK_OFS(i)	((i) >> 5)
#define MASK_BIT(i)	(1U << ((i) & 31))

static int snd_mask_single(const snd_mask_t *mask)
{
	int i, c = 0;
	for (i = 0; i < MASK_SIZE; i++) {
		if (!mask->bits[i])
			continue;
		if (mask->bits[i] & (mask->bits[i] - 1))
			return 0;
		if (c)
			return 0;
		c++;
	}
	return 1;
}

static void snd_mask_leave(snd_mask_t *mask, unsigned int val)
{
	unsigned int v;
	v = mask->bits[MASK_OFS(val)] & MASK_BIT(val);
	snd_mask_none(mask);
	mask->bits[MASK_OFS(val)] = v;
}

static void snd_mask_reset_range(snd_mask_t *mask, unsigned int from,
				 unsigned int to)
{
	unsigned int i;
	for (i = from; i <= to; i++)
		mask->bits[MASK_OFS(i)] &= ~MASK_BIT(i);
}

static int snd_mask_refine_min(snd_mask_t *mask, unsigned int val)
{
	if (snd_mask_empty(mask))
		return -ENOENT;
	if (snd_mask_min(mask) >= val)
		return 0;
	snd_mask_reset_range(mask, 0, val - 1);
	if (snd_mask_empty(mask))
		return -EINVAL;
	return 1;
}

static int snd_mask_refine_max(snd_mask_t *mask, unsigned int val)
{
	if (snd_mask_empty(mask))
		return -ENOENT;
	if (snd_mask_max(mask) <= val)
		return 0;
	snd_mask_reset_range(mask, val + 1, SND_MASK_MAX);
	if (snd_mask_empty(mask))
		return -EINVAL;
	return 1;
}

static int snd_mask_refine_set(snd_mask_t *mask, unsigned int val)
{
	int changed;
	if (snd_mask_empty(mask))
		return -ENOENT;
	changed = !snd_mask_single(mask);
	snd_mask_leave(mask, val);
	if (snd_mask_empty(mask))
		return -EINVAL;
	return changed;
}

static inline unsigned int ld2(u_int32_t v)
{
        unsigned r = 0;

        if (v >= 0x10000) {
                v >>= 16;
                r += 16;
        }
        if (v >= 0x100) {
                v >>= 8;
                r += 8;
        }
        if (v >= 0x10) {
                v >>= 4;
                r += 4;
        }
        if (v >= 4) {
                v >>= 2;
                r += 2;
        }
        if (v >= 2)
                r++;
        return r;
}

static unsigned int snd_mask_max(const snd_mask_t *mask)
{
	int i;
	for (i = MASK_SIZE - 1; i >= 0; i--) {
		if (mask->bits[i])
			return ld2(mask->bits[i]) + (i << 5);
	}
	return 0;
}

static unsigned int snd_mask_min(const snd_mask_t *mask)
{
	int i;
	for (i = 0; i < MASK_SIZE; i++) {
		if (mask->bits[i])
			return ffs(mask->bits[i]) - 1 + (i << 5);
	}
	return 0;
}

#define snd_mask_value(mask)	snd_mask_min(mask)

/*
 */

static void snd_interval_any(snd_interval_t *i)
{
	i->min = 0;
	i->openmin = 0;
	i->max = UINT_MAX;
	i->openmax = 0;
	i->integer = 0;
	i->empty = 0;
}

static inline void snd_interval_none(snd_interval_t *i)
{
	i->empty = 1;
}

static inline int snd_interval_empty(const snd_interval_t *i)
{
	return i->empty;
}

static inline int snd_interval_single(const snd_interval_t *i)
{
	return (i->min == i->max || 
		(i->min + 1 == i->max && i->openmax));
}

static inline int snd_interval_value(const snd_interval_t *i)
{
	return i->min;
}

static inline int snd_interval_min(const snd_interval_t *i)
{
	return i->min;
}

static inline int snd_interval_max(const snd_interval_t *i)
{
	return i->max;
}

static int snd_interval_setinteger(snd_interval_t *i)
{
	if (i->integer)
		return 0;
	if (i->openmin && i->openmax && i->min == i->max)
		return -EINVAL;
	i->integer = 1;
	return 1;
}

static inline int snd_interval_checkempty(const snd_interval_t *i)
{
	return (i->min > i->max ||
		(i->min == i->max && (i->openmin || i->openmax)));
}

static int snd_interval_refine_min(snd_interval_t *i, unsigned int min,
				   int openmin)
{
	int changed = 0;
	if (snd_interval_empty(i))
		return -ENOENT;
	if (i->min < min) {
		i->min = min;
		i->openmin = openmin;
		changed = 1;
	} else if (i->min == min && !i->openmin && openmin) {
		i->openmin = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmin) {
			i->min++;
			i->openmin = 0;
		}
	}
	if (snd_interval_checkempty(i)) {
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

static int snd_interval_refine_max(snd_interval_t *i, unsigned int max,
				   int openmax)
{
	int changed = 0;
	if (snd_interval_empty(i))
		return -ENOENT;
	if (i->max > max) {
		i->max = max;
		i->openmax = openmax;
		changed = 1;
	} else if (i->max == max && !i->openmax && openmax) {
		i->openmax = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmax) {
			i->max--;
			i->openmax = 0;
		}
	}
	if (snd_interval_checkempty(i)) {
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

static int snd_interval_refine(snd_interval_t *i, const snd_interval_t *v)
{
	int changed = 0;
	if (snd_interval_empty(i))
		return -ENOENT;
	if (i->min < v->min) {
		i->min = v->min;
		i->openmin = v->openmin;
		changed = 1;
	} else if (i->min == v->min && !i->openmin && v->openmin) {
		i->openmin = 1;
		changed = 1;
	}
	if (i->max > v->max) {
		i->max = v->max;
		i->openmax = v->openmax;
		changed = 1;
	} else if (i->max == v->max && !i->openmax && v->openmax) {
		i->openmax = 1;
		changed = 1;
	}
	if (!i->integer && v->integer) {
		i->integer = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmin) {
			i->min++;
			i->openmin = 0;
		}
		if (i->openmax) {
			i->max--;
			i->openmax = 0;
		}
	} else if (!i->openmin && !i->openmax && i->min == i->max)
		i->integer = 1;
	if (snd_interval_checkempty(i)) {
		snd_interval_none(i);
		return -EINVAL;
	}
	return changed;
}

static int snd_interval_refine_set(snd_interval_t *i, unsigned int val)
{
	snd_interval_t t;
	t.empty = 0;
	t.min = t.max = val;
	t.openmin = t.openmax = 0;
	t.integer = 1;
	return snd_interval_refine(i, &t);
}

static void snd_interval_print(const snd_interval_t *i, snd_output_t *out)
{
	if (snd_interval_empty(i))
		snd_output_printf(out, "NONE");
	else if (i->min == 0 && i->openmin == 0 && 
		 i->max == UINT_MAX && i->openmax == 0)
		snd_output_printf(out, "ALL");
	else if (snd_interval_single(i) && i->integer)
		snd_output_printf(out, "%u", snd_interval_value(i));
	else
		snd_output_printf(out, "%c%u %u%c",
				i->openmin ? '(' : '[',
				i->min, i->max,
				i->openmax ? ')' : ']');
}

/*
 */

static int snd_pcm_sw_params_default(snd_pcm_t *pcm,
				     snd_pcm_sw_params_t *params)
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

static inline
snd_mask_t *hw_param_mask(snd_pcm_hw_params_t *params,
			  snd_pcm_hw_param_t var)
{
	return (snd_mask_t*)&params->masks[var - SND_PCM_HW_PARAM_FIRST_MASK];
}

static inline
snd_interval_t *hw_param_interval(snd_pcm_hw_params_t *params,
				  snd_pcm_hw_param_t var)
{
	return &params->intervals[var - SND_PCM_HW_PARAM_FIRST_INTERVAL];
}

static inline
const snd_mask_t *hw_param_mask_c(const snd_pcm_hw_params_t *params,
				  snd_pcm_hw_param_t var)
{
	return (const snd_mask_t *)hw_param_mask((snd_pcm_hw_params_t*) params, var);
}

static inline
const snd_interval_t *hw_param_interval_c(const snd_pcm_hw_params_t *params,
					  snd_pcm_hw_param_t var)
{
	return (const snd_interval_t *)hw_param_interval((snd_pcm_hw_params_t*) params, var);
}

/*
 */
static void _snd_pcm_hw_param_any(snd_pcm_hw_params_t *params,
				  snd_pcm_hw_param_t var)
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
	params->rmasrmaskk = ~0U;
	params->cmask = 0;
	params->info = ~0U;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	_snd_pcm_hw_params_any(params);
	return snd_pcm_hw_refine(pcm, params);
}

int _snd_pcm_hw_param_get(const snd_pcm_hw_params_t *params,
			  int var, unsigned int *val, int *dir)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *mask = hw_param_mask_c(params, var);
		if (snd_mask_empty(mask) || !snd_mask_single(mask))
			return -EINVAL;
		if (dir)
			*dir = 0;
		if (val)
			*val = snd_mask_value(mask);
		return 0;
	} else if (hw_is_interval(var)) {
		const snd_interval_t *i = hw_param_interval_c(params, var);
		if (snd_interval_empty(i) || !snd_interval_single(i))
			return -EINVAL;
		if (dir)
			*dir = i->openmin;
		if (val)
			*val = snd_interval_value(i);
		return 0;
	}
	return -EINVAL;
}

/* Return the minimum value for field PAR. */
int _snd_pcm_hw_param_get_min(const snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *m = hw_param_mask_c(params, var);
		if (dir)
			*dir = 0;
		if (val)
			*val = snd_mask_min(m);
		return 0;
	} else if (hw_is_interval(var)) {
		const snd_interval_t *i = hw_param_interval_c(params, var);
		if (dir)
			*dir = i->openmin;
		if (val)
			*val = snd_interval_min(i);
		return 0;
	}
	return 0;
}

/* Return the maximum value for field PAR. */
int _snd_pcm_hw_param_get_max(const snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *m = hw_param_mask_c(params, var);
		if (dir)
			*dir = 0;
		if (val)
			*val = snd_mask_max(m);
		return 0;
	} else if (hw_is_interval(var)) {
		const snd_interval_t *i = hw_param_interval_c(params, var);
		if (dir)
			*dir = - (int) i->openmax;
		if (val)
			*val = snd_interval_max(i);
		return 0;
	}
	return 0;
}

static int hw_param_set_min(snd_pcm_hw_params_t *params,
			    int var, unsigned int val, int dir)
{
	int changed;
	int openmin = 0;
	if (dir) {
		if (dir > 0) {
			openmin = 1;
		} else if (dir < 0) {
			if (val > 0) {
				openmin = 1;
				val--;
			}
		}
	}
	if (hw_is_mask(var))
		changed = snd_mask_refine_min(hw_param_mask(params, var), val + !!openmin);
	else if (hw_is_interval(var))
		changed = snd_interval_refine_min(hw_param_interval(params, var), val, openmin);
	else {
		return -EINVAL;
	}
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

int _snd_pcm_hw_param_set_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	snd_pcm_hw_params_t save;
	int err;

	save = *params;
	err = hw_param_set_min(params, var, *val, dir ? *dir : 0);
	if (err < 0)
		goto _fail;
	if (params->rmask) {
		err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			goto _fail;
		if (snd_pcm_hw_param_empty(params, var)) {
			err = -ENOENT;
			goto _fail;
		}
	}
	return snd_pcm_hw_param_get_min(params, var, val, dir);
 _fail:
	*params = save;
	return err;
}

static int hw_param_set_max(snd_pcm_hw_params_t *params,
			    int var, unsigned int val, int dir)
{
	int changed;
	int openmax = 0;
	if (dir) {
		if (dir < 0) {
			openmax = 1;
		} else if (dir > 0) {
			openmax = 1;
			val++;
		}
	}
	if (hw_is_mask(var)) {
		if (val == 0 && openmax) {
			snd_mask_none(hw_param_mask(params, var));
			changed = -EINVAL;
		} else
			changed = snd_mask_refine_max(hw_param_mask(params, var), val - !!openmax);
	} else if (hw_is_interval(var))
		changed = snd_interval_refine_max(hw_param_interval(params, var), val, openmax);
	else {
		return -EINVAL;
	}
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

int _snd_pcm_hw_param_set_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	snd_pcm_hw_params_t save;
	int err;

	save = *params;
	err = hw_param_set_max(params, var, *val, dir ? *dir : 0);
	if (err < 0)
		goto _fail;
	if (params->rmask) {
		err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			goto _fail;
		if (snd_pcm_hw_param_empty(params, var)) {
			err = -ENOENT;
			goto _fail;
		}
	}
	return snd_pcm_hw_param_get_max(params, var, val, dir);
 _fail:
	*params = save;
	return err;
}

static int hw_param_set_minmax(snd_pcm_hw_params_t *params,
			       int var,
			       unsigned int min, int mindir,
			       unsigned int max, int maxdir)
{
	int changed, c1, c2;
	int openmin = 0, openmax = 0;
	if (mindir) {
		if (mindir > 0) {
			openmin = 1;
		} else if (mindir < 0) {
			if (min > 0) {
				openmin = 1;
				min--;
			}
		}
	}
	if (maxdir) {
		if (maxdir < 0) {
			openmax = 1;
		} else if (maxdir > 0) {
			openmax = 1;
			max++;
		}
	}
	if (hw_is_mask(var)) {
		snd_mask_t *mask = hw_param_mask(params, var);
		if (max == 0 && openmax) {
			snd_mask_none(mask);
			changed = -EINVAL;
		} else {
			c1 = snd_mask_refine_min(mask, min + !!openmin);
			if (c1 < 0)
				changed = c1;
			else {
				c2 = snd_mask_refine_max(mask, max - !!openmax);
				if (c2 < 0)
					changed = c2;
				else
					changed = (c1 || c2);
			}
		}
	}
	else if (hw_is_interval(var)) {
		snd_interval_t *i = hw_param_interval(params, var);
		c1 = snd_interval_refine_min(i, min, openmin);
		if (c1 < 0)
			changed = c1;
		else {
			c2 = snd_interval_refine_max(i, max, openmax);
			if (c2 < 0)
				changed = c2;
			else
				changed = (c1 || c2);
		}
	} else {
		return -EINVAL;
	}
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

int _snd_pcm_hw_param_set_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
				 int var,
				 unsigned int *min, int *mindir,
				 unsigned int *max, int *maxdir)
{
	snd_pcm_hw_params_t save;
	int err;

	save = *params;
	err = hw_param_set_minmax(params, var, 
				  *min, mindir ? *mindir : 0,
				  *max, maxdir ? *maxdir : 0);
	if (err < 0)
		goto _fail;
	if (params->rmask) {
		err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			goto _fail;
	}
	err = _snd_pcm_hw_param_get_min(params, var, min, mindir);
	if (err < 0)
		return err;
	return snd_pcm_hw_param_get_max(params, var, max, maxdir);
 _fail:
	*params = save;
	return err;
}

static int hw_param_set(snd_pcm_hw_params_t *params,
			int var, unsigned int val, int dir)
{
	int changed;
	if (hw_is_mask(var)) {
		snd_mask_t *m = hw_param_mask(params, var);
		if (val == 0 && dir < 0) {
			changed = -EINVAL;
			snd_mask_none(m);
		} else {
			if (dir > 0)
				val++;
			else if (dir < 0)
				val--;
			changed = snd_mask_refine_set(hw_param_mask(params, var), val);
		}
	} else if (hw_is_interval(var)) {
		snd_interval_t *i = hw_param_interval(params, var);
		if (val == 0 && dir < 0) {
			changed = -EINVAL;
			snd_interval_none(i);
		} else if (dir == 0)
			changed = snd_interval_refine_set(i, val);
		else {
			snd_interval_t t;
			t.openmin = 1;
			t.openmax = 1;
			t.empty = 0;
			t.integer = 0;
			if (dir < 0) {
				t.min = val - 1;
				t.max = val;
			} else {
				t.min = val;
				t.max = val+1;
			}
			changed = snd_interval_refine(i, &t);
		}
	} else {
		return -EINVAL;
	}
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}

int _snd_pcm_hw_param_test(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			   int var, unsigned int val, int dir)
{
	snd_pcm_hw_params_t save = *params;
	return hw_param_set(&save, var, val, dir);
}

int _snd_pcm_hw_param_set(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			  int mode, int var, unsigned int val, int dir)
{
	snd_pcm_hw_params_t save = *params;
	int err = hw_param_set(params, var, val, dir);
	if (err < 0)
		goto _fail;
	if (params->rmask) {
		err = snd_pcm_hw_refine(pcm, params);
		if (err < 0)
			goto _fail;
	}
	return 0;
 _fail:
	*params = save;
	return err;
}

int _snd_pcm_hw_param_set_integer(snd_pcm_t *pcm, 
				  snd_pcm_hw_params_t *params,
				  int var)
{
	snd_pcm_hw_params_t save;
	save = *params;
	err = snd_interval_setinteger(hw_param_interval(params, var));
	if (err < 0)
		return err;
	if (err) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	err = snd_pcm_hw_refine(pcm, params);
	if (err < 0)
		*params = save;
	return err;
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

int snd_pcm_hw_params_get_min_align(const snd_pcm_hw_params_t *params,
				    snd_pcm_uframes_t *val)
{
	unsigned int format, channels, fb, min_align;
	int err;

	err = _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_FORMAT,
				    &format, NULL);
	if (err < 0)
		return err;
	err = _snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_CHANNELS,
				    &channels, NULL);
	if (err < 0)
		return err;
	/* compute frame bits */
	fb = snd_pcm_format_physical_width(format) * channels;
        min_align = 1;
	while (fb % 8) {
		fb *= 2;
                min_align *= 2;
	}
	if (val)
		*val = min_align;
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
