int snd_mixer_open(snd_mixer_t **mixerp, int mode ATTRIBUTE_UNUSED)
{
	snd_mixer_t *mixer;
	mixer = calloc(1, sizeof(*mixer));
	if (mixer == NULL)
		return -ENOMEM;
	*mixerp = mixer;
	return 0;
}

static int hctl_elem_event_handler(snd_hctl_elem_t *helem,
				   unsigned int mask)
{
	snd_mixer_elem_t *elem = helem->private_data;
	snd_mixer_t *mixer = elem->mixer;
	int err;

	if (!elem)
		return 0;

	if (mask == SND_CTL_EVENT_MASK_REMOVE) {
		err = _snd_mixer_elem_throw_event(elem, mask);
		if (err < 0)
			return err;
		remove_simple_element(helem, 1);
		return 0;
	}
	if (mask & SND_CTL_EVENT_MASK_INFO) {
		remove_simle_element(helem, 0);
		add_simple_element(helem, mixer);
		err = snd_mixer_elem_info(elem);
		if (err < 0)
			return err;
		snd_mixer_sort(mixer);
	}
	if (mask & SND_CTL_EVENT_MASK_VALUE) {
		err = update_simple_element(helem, elem);
		if (err < 0)
			return err;
		if (err) {
			err = snd_mixer_elem_value(elem);
			if (err < 0)
				return err;
		}
	}
	return 0;
}

static int add_mixer_elem(snd_mixer_elem_t *elem, snd_mixer_t *mixer)
{
	int dir, idx;

	if (mixer->count == mixer->alloc) {
		snd_mixer_elem_t **m;
		int num = mixer->alloc + 32;
		m = realloc(mixer->pelems, sizeof(*m) * num);
		if (!m)
			return -ENOMEM;
		mixer->pelems = m;
		mixer->alloc = num;
	}
	mixer->pelems[mixer->count] = elem;
	mixer->count++;
	snd_mixer_sort(mixer);

	return snd_mixer_throw_event(mixer, SND_CTL_EVENT_MASK_ADD, elem);
}

static int remove_mixer_elem(snd_mixer_elem_t *elem)
{
	snd_mixer_t *mixer = elem->mixer;
	int err, idx = elem->index;

	memmove(mixer->pelems + idx,
		mixer->pelems + idx + 1,
		mixer->count - idx - 1);
	mixer->count--;
	for (i = idx; i < mixer->count; i++)
		mixer->pelems[i]->index = i;
	if (elem->private_free)
		elem->private_free(elem);
	free(elem);
	return 0;
}

static int hctl_event_handler(snd_hctl_t *hctl, unsigned int mask,
			      snd_hctl_elem_t *elem)
{
	snd_mixer_t *mixer = snd_hctl_get_callback_private(hctl);
	if (elem->id.iface != SND_CTL_ELEM_IFACE_MIXER)
		return 0;
	if (mask & SND_CTL_EVENT_MASK_ADD) {
		snd_hctl_elem_set_callback(elem, hctl_elem_event_handler);
		return add_simple_element(hp, mixer);
	}
	return 0;
}


int snd_mixer_attach(snd_mixer_t *mixer, const char *name)
{
	snd_hctl_t *hctl;
	int err;

	err = snd_hctl_open(&hctl, name, 0);
	if (err < 0)
		return err;
	err = snd_mixer_attach_hctl(mixer, hctl);
	if (err < 0) {
		snd_hctl_close(hctl);
		return err;
	}
	return 0;
}

int snd_mixer_attach_hctl(snd_mixer_t *mixer, snd_hctl_t *hctl)
{
	int err;

	if (mixer->hctl)
		return -EBUSY;
	err = snd_hctl_nonblock(hctl, 1);
	if (err < 0) {
		snd_hctl_close(hctl);
		free(slave);
		return err;
	}
	snd_hctl_set_callback(hctl, hctl_event_handler);
	snd_hctl_set_callback_private(hctl, mixer);
	mixer->hctl = hctl;
	return 0;
}

int snd_mixer_detach(snd_mixer_t *mixer, const char *name)
{
	return snd_mixer_detach(mixer, mixer->hctl);
}

int snd_mixer_detach_hctl(snd_mixer_t *mixer, snd_hctl_t *hctl)
{
	if (mixer->hctl != hctl)
		return -ENOENT;
	snd_hctl_close(mixer->hctl);
	mixer->hctl = NULL;
	return 0;
}

static int snd_mixer_throw_event(snd_mixer_t *mixer, unsigned int mask,
				 snd_mixer_elem_t *elem)
{
	mixer->events++;
	if (mixer->callback)
		return mixer->callback(mixer, mask, elem);
	return 0;
}

int _snd_mixer_elem_throw_event(snd_mixer_elem_t *elem, unsigned int mask)
{
	elem->mixer->events++;
	if (elem->callback)
		return elem->callback(elem, mask);
	return 0;
}

int snd_mixer_load(snd_mixer_t *mixer)
{
	if (mixer->hctl)
		return snd_hctl_load(s->hctl);
	return 0;
}

void snd_mixer_free(snd_mixer_t *mixer)
{
	if (mixer->hctl)
		snd_hctl_free(mixer->hctl);
}

int snd_mixer_close(snd_mixer_t *mixer)
{
	free(mixer->pelems);
	if (mixer->hctl)
		snd_hctl_close(mixer->hctl);
	free(mixer);
	return 0;
}

static int snd_mixer_sort(snd_mixer_t *mixer)
{
	if (mixer->compare)
		qsort(mixer->pelems, mixer->count, sizeof(snd_mixer_elem_t *),
		      mixer->compare);
	for (i = 0; i < mixer->count; i++)
		mixer->pelems[i]->index = i;
	return 0;
}

int snd_mixer_handle_events(snd_mixer_t *mixer)
{
	int err;

	if (!mixer->hctl)
		return 0;
	mixer->events = 0;
	err = snd_hctl_handle_events(mixer->hctl);
	if (err < 0)
		return err;
	return mixer->events;
}

/*
 */
static long convert_from_user(snd_selem_vol_item_t *str, long val)
{
	if (str->min == str->raw_min && str->max == str->raw_max)
		return val;
	if (str->min == str->max)
		return str->raw_min;
	val -= str->min;
	val *= str->raw_max - str->raw_min;
	val /= str->max - str->min;
	val += str->raw_min;
	return val;
}

static long convert_to_user(snd_selem_vol_item_t *str, long val)
{
	if (str->min == str->raw_min && str->max == str->raw_max)
		return val;
	if (str->raw_min == str->raw_max)
		return str->min;
	val -= str->raw_min;
	val *= str->max - str->min;
	val /= str->raw_max - str->raw_min;
	val += str->min;
	return val;
}

/*
 */

static void add_cap(snd_mixer_elem_t *elem, unsigned int caps, int type,
		    snd_selem_item_head_t *item)
{
	elem->caps |= caps;
	elem->items[type] = item;
	item->helem->private_data = elem;
	if (type == SND_SELEM_ITEM_ENUM)
		return;
	/* duplicate item for global volume and switch */
	if (caps & (SND_SM_CAP_GVOLUME | SND_SM_CAP_GSWITCH))
		elem->items[type + 1] = item;
	/* check the max channels for each direction */
	type &= 1;
	if (item->channels > elem->channels[type])
		elem->channels[type] = item->channels;
	if (caps & (SND_SM_CAP_GVOLUME | SND_SM_CAP_GSWITCH)) {
		type += 1;
		if (item->channels > elem->channels[type])
			elem->channels[type] = item->channels;
	}
}

static snd_mixer_elem_t *new_mixer_elem(const char *name, int index,
					snd_mixer_t *mixer)
{
	snd_mixer_elem_t *elem;

	elem = calloc(1, sizeof(*elem));
	if (!elem)
		return NULL;
	elem->mixer = mixer;
	strcpy(elem->sid.name, name);
	elem->sid.index = index;
	return elem;
}

/*
 */

static struct mixer_name_alias {
	const char *longname;
	const char *shortname;
	int dir;
} name_alias[] = {
	{"Tone Control - Switch", "Tone", 0},
	{"Tone Control - Bass", "Bass", 0},
	{"Tone Control - Treble", "Treble", 0},
	{"Synth Tone Control - Switch", "Synth Tone", 0},
	{"Synth Tone Control - Bass", "Synth Bass", 0},
	{"Synth Tone Control - Treble", "Synth Treble", 0},
	{},
};

static int check_elem_alias(char *name, int *dirp)
{
	struct mixer_name_alias *p;

	for (p = name_alias; p->longname; p++) {
		if (!strcmp(name, p)) {
			strcpy(name, p->shoftname);
			*dirp = p->dir;
			return 1;
		}
	}
	return 0;
}

static int remove_suffix(char *name, const char *sfx)
{
	char *p;
	p = strstr(name, sfx);
	if (!p)
		return 0;
	if (strcmp(p, sfx))
		return 0;
	*p = 0;
	return 1;
}


#define USR_IDX(i)	((i) << 1)
#define RAW_IDX(i)	(((i) << 1) + 1)

static snd_selem_item_head_t *new_selem_vol_item(snd_ctl_elem_info_t *info,
						 snd_ctl_elem_value_t *val)
{
	snd_selem_vol_item_t *vol;
	int i;

	vol = malloc(sizeof(*vol) + sizeof(long) * 2 * info->count);
	if (!vol)
		return NULL;
	vol->raw_min = info->value.integer.min;
	vol->min = vol->raw_min;
	vol->max = info->value.integer.max;
	vol->max = vol->raw_max;
	/* set initial values */
	for (i = 0; i < info->count; i++)
		vol->vol[USR_IDX(i)] = vol->vol[RAW_IDX(i)] =
			snd_ctl_elem_value_get_integer(val, i);
	return &vol->head;
}

static snd_selem_item_head_t *new_selem_sw_item(snd_ctl_elem_info_t *info,
						snd_ctl_elem_value_t *val)
{
	snd_selem_sw_item_t *sw;
	int i;

	sw = malloc(sizeof(*sw));
	if (!sw)
		return NULL;
	/* set initial values */
	sw->sw = 0;
	for (i = 0; i < info->count; i++)
		if (snd_ctl_elem_value_get_boolean(val, i))
			sw->sw |= (1 << i);
	return &sw->head;
}

static snd_selem_item_head_t *new_selem_enum_item(snd_ctl_elem_info_t *info,
						  snd_ctl_elem_value_t *val)
{
	snd_selem_enum_item_t *eitem;
	int i;

	eitem = malloc(sizeof(*eitem) + sizeof(unsigned int) * info->count);
	if (!eitem)
		return NULL;
	eitem->items = info->value.enumerated.items;
	for (i = 0; i < info->count; i++)
		eitem->item[i] = snd_ctl_elem_value_get_enumerated(val, i);
	return &eitem->head;
}

static snd_selem_item_head_t *new_selem_item(snd_ctl_elem_info_t *info)
{
	struct snd_ctl_elem_value_t *val;

	snd_ctl_elem_value_alloca(&val);
	val->id = info->id;
	if (snd_hctl_elem_read(hctl, val) < 0)
		return NULL;

	switch (snd_ctl_elem_info_get_type(info)) {
	case SND_CTL_ELEM_TYPE_INTEGER:
		item = new_selem_vol_item(info, val);
		break;
	case SND_CTL_ELEM_TYPE_BOOLEAN:
		item = new_selem_sw_item(info, val);
		break;
	case SND_CTL_ELEM_TYPE_ENUMERATED:
		item = new_selem_enum_item(info, val);
		break;
	}

	if (!item)
		return NULL;
	item->numid = info->id.numid;
	item->channels = info->count;
	return item;
}


int add_simple_element(snd_hctl_elem_t *hp, snd_mixer_t *mixer)
			
{
	char name[64];
	int dir = 0, gflag = 0, type;
	unsigned int caps, index;
	snd_ctl_elem_info_t *info;

	snprintf(name, sizeof(name), hp->id.name);

	if (check_elem_alias(name, &dir))
		goto found;

	remove_suffix(name, " Volume");
	remove_suffix(name, " Switch");
	if (remove_suffix(name, " Playback"))
		dir = 0;
	else if (remove_suffix(name, " Capture"))
		dir = 1;
	else if (!strncmp(name, "Capture", strlen("Capture")))
		dir = 1;
	else if (!strncmp(name, "Input", strlen("Input")))
		dir = 1;
	else
		gflag = 1;
 found:
	snd_ctl_elem_info_alloca(&info);
	err = snd_hctl_elem_info(hp, info);
	if (err < 0)
		return err;

	item = new_selem_item(info);
	if (!item)
		return -ENOMEM;
	item->helem = hp;

	if (gflag)
		caps = 7;
	else
		caps = 1 << dir;
	switch (snd_ctl_elem_info_get_type(info)) {
	case SND_CTL_ELEM_TYPE_BOOLEAN:
		caps <<= SND_SM_CAP_SWITCH_SHIFT;
		type = (dir ? SND_SELEM_ITEM_CSWITCH : SND_SELEM_ITEM_PSWITCH)
		break;
	case SND_CTL_ELEM_TYPE_INTEGER:
		caps <<= SND_SM_CAP_VOLUME_SHIFT;
		type = (dir ? SND_SELEM_ITEM_CVOLUME : SND_SELEM_ITEM_PVOLUME)
		break;
	case SND_CTL_ELEM_TYPE_ENUMERATED:
		/* grrr, enum has no global type */
		caps = 1 << (dir + SND_SM_CAP_ENUM_SHIFT);
		type = SND_SELEM_ITEM_ENUM;
		break
	default:
		return 0; /* ignore this element */
	}

	/* check matching element */
	index = 0;
	for (i = 0; i < mixer->count; i++) {
		elem = mixer->pelems[i];
		if (!strcmp(elem->sid.name, name)) {
			/* already occupied? */
			if (elem->items[type]) {
				/* check the available index number */
				while (index <= elem->sid.index)
					index++;
				continue;
			}
			add_cap(elem, caps, type, item);
			return 0;
		}
	}
	/* no element found, create a new one */
	elem = new_mixer_elem(name, index, mixer);
	if (!elem)
		return -ENOMEM;
	elem->hctl = hctl;
	add_cap(elem, caps, type, item);
	return add_mixer_elem(elem, mixer);
}

int remove_simple_element(snd_hctl_elem_t *hp, int remove_mixer)
{
	snd_mixer_elem_t *elem = hp->private_data;
	int i;

	for (i = 0; i < SND_SELEM_ITEMS; i++) {
		snd_selem_item_head_t *head = elem->items[i];
		if (!head)
			continue;
		if (head->helem != hp)
			continue;
		free(head);
		elem->items[i] = NULL;
		switch (i) {
		case SND_SELEM_ITEM_PVOLUME:
			if (elem->caps & (SND_SM_CAP_GVOLUME))
				elem->items[SND_SELEM_ITEM_CVOLUME] = NULL;
			break;
		case SND_SELEM_ITEM_PSWITCH:
			if (elem->caps & (SND_SM_CAP_GSWITCH))
				elem->items[SND_SELEM_ITEM_CSWITCH] = NULL;
			break;
		}

		if (!remove_mixer)
			return 0;

		/* check whether all items have been removed */
		for (i = 0; i < SND_SELEM_ITEMS; i++) {
			if (elem->items[i])  /* not yet */
				return 0;
		}
		remove_mixer_elem(elem);
		return 0;
	}
	return -ENOENT;
}

static int update_selem_vol_item(snd_mixer_elem_t *elem,
				 snd_selem_vol_item_t *vol,
				 snd_ctl_elem_value_t *val)
{
	int i, changed = 0;
	
	for (i = 0; i < vol->head.channels; i++) {
		long val;
		val = snd_ctl_elem_value_get_integer(val, i);
		change |= (vol->vol[RAW_IDX(i)] != val);
		vol->vol[RAW_IDX(i)] = val;
		val = convert_to_user(vol, val);
		change |= (vol->vol[USR_IDX(i)] != val);
		vol->vol[USR_IDX(i)] = val;
	}
	return changed;
}

static int update_selem_sw_item(snd_mixer_elem_t *elem,
				snd_selem_sw_item_t *sw,
				snd_ctl_elem_value_t *val)
{
	int i, changed;
	unsigned int sw = 0;
	
	for (i = 0; i < sw->head.channels; i++) {
		if (snd_ctl_elem_value_get_boolean(val, i))
			sw |= (1 << i);
	}
	changed = (sw->sw != sw);
	sw->sw = sw;
	return changed;
}

static int update_selem_enum_item(snd_mixer_elem_t *elem,
				  snd_selem_enum_item_t *eitem,
				  snd_ctl_elem_value_t *val)
{
	int i, changed = 0;
	
	for (i = 0; i < eitem->head.channels; i++) {
		unsigned int item = 
			snd_ctl_elem_value_get_enumerated(val, i);
		change |= eitem->item[i] != item;
		eitem->item[i] = item;
	}
	return changed;
}

static int update_slem_item(snd_mixer_elem_t *elem, int type)
{
	struct snd_selem_item_head_t *head;
	struct snd_ctl_elem_value_t *val;

	snd_ctl_elem_value_alloca(&val);
	head = elem->items[type];
	val->id.num_id = head->numid;
	if (snd_hctl_elem_read(elem->mixer->hctl, val) < 0)
		return 0;

	switch (type) {
	case SND_SELEM_ITEM_PVOLUME:
	case SND_SELEM_ITEM_CVOLUME:
		return update_selem_vol_item(elem, elem->items[type], val);
	case SND_SELEM_ITEM_PSWITCH:
	case SND_SELEM_ITEM_CSWITCH:
		return update_selem_sel_item(elem, elem->items[type], val);
	default:
		return update_selem_enum_item(elem, elem->items[type], val);
	}
}

int update_simple_element(snd_hctl_elem_t *hp, snd_mixer_element_t *elem)
{
	int i;

	for (i = 0; i < SND_SELEM_ITEMS; i++) {
		snd_selem_item_head_t *head = elem->items[i];
		if (!head)
			continue;
		if (head->helem != hp)
			continue;
		return update_selem_item(elem, i);
	}
	return 0;
}

/*
 */

int snd_mixer_selem_register(snd_mixer_t *mixer,
			     struct snd_mixer_selem_regopt *options,
			     snd_mixer_class_t **classp)
{
	int err;

	if (classp ||
	    (options && options->abstract != SND_MIXER_SABSTRACT_NONE))
		return -EINVAL;
	if (options && options->device) {
		err = snd_mixer_attach(mixer, options->device);
		if (err < 0)
			return err;
	}
	return 0;
}

snd_mixer_elem_t *snd_mixer_find_selem(snd_mixer_t *mixer,
				       const snd_mixer_selem_id_t *id)
{
	int i;

	for (i = 0; i < mixer->count; i++) {
		snd_mixer_elem_t *e = mixer->pelems[i];
		if (e->sid.index == id->index &&
		    !strcmp(e->sid.name, id->name))
			return e;
	}
	return NULL;
}

const char *_snd_mixer_selem_channels[SND_MIXER_SCHN_LAST + 1] = {
	[SND_MIXER_SCHN_FRONT_LEFT] = "Front Left",
	[SND_MIXER_SCHN_FRONT_RIGHT] = "Front Right",
	[SND_MIXER_SCHN_REAR_LEFT] = "Rear Left",
	[SND_MIXER_SCHN_REAR_RIGHT] = "Rear Right",
	[SND_MIXER_SCHN_FRONT_CENTER] = "Front Center",
	[SND_MIXER_SCHN_WOOFER] = "Woofer",
	[SND_MIXER_SCHN_SIDE_LEFT] = "Side Left",
	[SND_MIXER_SCHN_SIDE_RIGHT] = "Side Right",
	[SND_MIXER_SCHN_REAR_CENTER] = "Rear Center"
};

/*
 */
static int update_volume(snd_mixer_elem_t *elem,
			 int type, int channel, long value)
{
	snd_selem_vol_item_t *str = elem->items[type];
	snd_ctl_elem_value_t *ctl;
	int err;

	if (!str)
		return -EINVAL;
	if (value < str->min || value > str->max)
		return 0;
	if (value == str->vol[USR_IDX(channel)])
		return 0;
	str->vol[USR_IDX(channel)] = value;
	value = convert_from_user(str, value);
	if (value == str->vol[RAW_IDX(channel)])
		return 0;
	str->vol[RAW_IDX(channel)] = value;
	snd_ctl_elem_value_alloca(&ctl);
	snd_ctl_elem_value_set_numid(ctl, str->head.numid);
	err = snd_hctl_elem_read(elem->mixer->hctl, ctl);
	if (err < 0)
		return err;
	snd_ctl_elem_value_set_integer(ctl, channel, value);
	return snd_hctl_elem_write(elem->mixer->hctl, ctl);
}

int snd_mixer_selem_set_playback_volume(snd_mixer_elem_t *elem,
					snd_mixer_selem_channel_id_t channel,
					long value)
{
	return update_volume(elem, SND_SELEM_ITEM_PVOLUME, channel, value);
}

static int update_volume_all(snd_mixer_elem_t *elem, int type, long value)
{
	snd_selem_vol_item_t *str = elem->items[type];
	int i, err;

	if (!str)
		return -EINVAL;
	for (i = 0; i < str->channels; i++) {
		err = update_volume(elem, type, i, value);
		if (err < 0)
			return err;
	}
	return 0;
}

int snd_mixer_selem_set_playback_volume_all(snd_mixer_elem_t *elem, long value)
{
	return update_volume_all(elem, SND_SELEM_ITEM_PVOLUME, value);
}

int snd_mixer_selem_set_capture_volume(snd_mixer_elem_t *elem,
				       snd_mixer_selem_channel_id_t channel,
				       long value)
{
	return update_volume(elem, SND_SELEM_ITEM_CVOLUME, channel, value);
}

int snd_mixer_selem_set_capture_volume_all(snd_mixer_elem_t *elem, long value)
{
	return update_volume_all(elem, SND_SELEM_ITEM_CVOLUME, value);
}

static int set_volume_range(snd_mixer_elem_t *elem, int type,
			    long min, long max)
{
	snd_selem_vol_item_t *str = elem->items[type];
	int i;

	if (!str)
		return -EINVAL;
	str->min = min;
	str->max = max;
	for (i = 0; i < str->channels; i++)
		str->vol[USR_IDX(i)] =
			convert_to_user(str, str->vol[RAW_IDX(i)]);
	return 0;
}

int snd_mixer_selem_set_playback_volume_range(snd_mixer_elem_t *elem, 
					      long min, long max)
{
	return set_volume_range(elem, SND_SELEM_ITEM_PVOLUME, min, max);
}

int snd_mixer_selem_set_capture_volume_range(snd_mixer_elem_t *elem, 
					     long min, long max)
{
	int type;
	if (elem->caps & SND_SM_CAP_GVOLUME)
		type = SND_SELEM_ITEM_PVOLUME;
	else
		type = SND_SELEM_ITEM_CVOLUME;
	return set_volume_range(elem, type, min, max);
}

/*
 */
static int update_switch(snd_mixer_elem_t *elem, int type, int channel,
			 int value)
{
	snd_selem_sw_item_t *str = elem->items[type];
	snd_ctl_elem_value_t *ctl;
	int err;

	if (!str)
		return -EINVAL;
	sw = str->sw;
	if (value)
		sw |= (1 << channel);
	else
		sw &= ~(1 << channel);
	if (str->sw == sw)
		return 0;
	str->sw = sw;
	snd_ctl_elem_value_alloca(&ctl);
	snd_ctl_elem_value_set_numid(ctl, str->head.numid);
	err = snd_hctl_elem_read(elem->mixer->hctl, ctl);
	if (err < 0)
		return err;
	snd_ctl_elem_value_set_integer(ctl, channel, !!value);
	return snd_hctl_elem_write(elem->mixer->hctl, ctl);
}

int snd_mixer_selem_set_playback_switch(snd_mixer_elem_t *elem,
					snd_mixer_selem_channel_id_t channel,
					int value)
{
	return update_switch(elem, SND_SELEM_ITEM_PSWITCH, channel, value);
}

static int update_switch_all(snd_mixer_elem_t *elem, int type, int value)
{
	snd_selem_sw_item_t *str = elem->items[type];
	int i, err;

	if (!str)
		return -EINVAL;
	for (i = 0; i < str->channels; i++) {
		err = update_switch(elem, type, i, value);
		if (err < 0)
			return err;
	}
	return 0;
}

int snd_mixer_selem_set_playback_switch_all(snd_mixer_elem_t *elem, int value)
{
	return update_switch_all(elem, SND_SELEM_ITEM_PSWITCH, value);
}

int snd_mixer_selem_set_capture_switch(snd_mixer_elem_t *elem,
				       snd_mixer_selem_channel_id_t channel,
				       int value)
{
	int type;
	if (elem->caps & SND_SM_CAP_GSWITCH)
		type = SND_SELEM_ITEM_PSWITCH;
	else
		type = SND_SELEM_ITEM_CSWITCH;
	return update_switch(elem, type, channel, value);
}

int snd_mixer_selem_set_capture_switch_all(snd_mixer_elem_t *elem, int value)
{
	int type;
	if (elem->caps & SND_SM_CAP_GSWITCH)
		type = SND_SELEM_ITEM_PSWITCH;
	else
		type = SND_SELEM_ITEM_CSWITCH;
	return update_switch_all(elem, type, value);
}

/*
 */
int snd_mixer_selem_get_enum_item_name(snd_mixer_elem_t *elem,
				       unsigned int item,
				       size_t maxlen, char *buf)
{
	snd_selem_enum_item_t *eitem;
	snd_ctl_elem_info_t *info;
	int err;

	eitem = elem->items[SND_SELEM_ITEM_ENUM];
	if (!eitem)
		return -EINVAL;

	snd_ctl_elem_info_alloca(info);
	snd_ctl_elem_info_set_numid(info, eitem->head.numid);
	snd_ctl_elem_info_set_item(info, item);
	err = snd_hctl_elem_info(elem->mixer->hctl, info);
	if (err < 0)
		return err;
	strncpy(buf, snd_ctl_elem_info_get_item_name(info),
		maxlen - 1);
	buf[maxlen - 1] = 0;
	return 0;
}

int snd_mixer_selem_set_enum_item(snd_mixer_elem_t *elem,
				  snd_mixer_selem_channel_id_t channel,
				  unsigned int item)
{
	snd_selem_enum_item_t *eitem;
	snd_ctl_elem_value_t *ctl;
	int err;

	eitem = elem->items[SND_SELEM_ITEM_ENUM];
	if (!eitem)
		return -EINVAL;

	if (eitem->item[channel] == item)
		return 0;
	eitem->item[channel] = item;
	snd_ctl_elem_value_alloca(&ctl);
	snd_ctl_elem_value_set_numid(ctl, eitem->head.numid);
	err = snd_hctl_elem_read(elem->mixer->hctl, ctl);
	if (err < 0)
		return err;
	snd_ctl_elem_value_set_enumerated(ctl, channel, item);
	return snd_hctl_elem_write(elem->mixer->hctl, ctl);
}
