#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "control.h"
#include "local.h"

#define SND_FILE_CONTROL	DEVPATH "/controlC%i"

int snd_card_load(int card)
{
	char control[sizeof(SND_FILE_CONTROL) + 10];
	sprintf(control, SND_FILE_CONTROL, card);
	return !access(control, R_OK);
}

int snd_card_next(int *rcard)
{
	int card;
	
	if (rcard == NULL)
		return -EINVAL;
	card = *rcard;
	card = card < 0 ? 0 : card + 1;
	for (; card < 32; card++) {
		if (snd_card_load(card)) {
			*rcard = card;
			return 0;
		}
	}
	*rcard = -1;
	return 0;
}

int _snd_ctl_hw_open(snd_ctl_t **ctlp, int card)
{
	char name[16];
	sprintf(name, "hw:%d", card);
	return snd_ctl_open(ctlp, name, 0);
}

static int get_card_info(int card, snd_ctl_card_info_t *info)
{
	int err;
	snd_ctl_t *handle;

	memset(info, 0, sizeof(*info));
	err = _snd_ctl_hw_open(&handle, card);
	if (err < 0)
		return err;
	err = snd_ctl_card_info(handle, info);
	snd_ctl_close(handle);
	return err;
}

int snd_card_get_index(const char *string)
{
	int card;
	snd_ctl_card_info_t info;

	if (!string || *string == '\0')
		return -EINVAL;
	if ((isdigit(*string) && *(string + 1) == 0) ||
	    (isdigit(*string) && isdigit(*(string + 1)) && *(string + 2) == 0)) {
		if (sscanf(string, "%i", &card) != 1)
			return -EINVAL;
		if (card < 0 || card > 31)
			return -EINVAL;
		if (snd_card_load(card))
			return card;
		return -ENODEV;
	}
	for (card = 0; card < 32; card++) {
		if (!snd_card_load(card))
			continue;
		if (get_card_info(card, &info) < 0)
			continue;
		if (!strcmp((const char *)info.id, string))
			return card;
	}
	return -ENODEV;
}

int snd_card_get_name(int card, char **name)
{
	snd_ctl_card_info_t info;
	int err;
	
	err = get_card_info(card, &info);
	if (err < 0)
		return err;
	*name = strdup((const char *)info.name);
	if (*name == NULL)
		return -ENOMEM;
	return 0;
}

int snd_card_get_longname(int card, char **name)
{
	snd_ctl_card_info_t info;
	int err;
	
	err = get_card_info(card, &info);
	if (err < 0)
		return err;
	*name = strdup((const char *)info.longname);
	if (*name == NULL)
		return -ENOMEM;
	return 0;
}

int _snd_dev_get_device(const char *name, int *cardp, int *devp, int *subdevp)
{
	int card;
	*cardp = 0;
	if (devp)
		*devp = 0;
	if (subdevp)
		*subdevp = -1;
	if (!strcmp(name, "hw") || !strcmp(name, "default"))
		return 0;
	if (!strncmp(name, "hw:", 3))
		name += 3;
	else if (!strncmp(name, "default:", 8))
		name += 8;
	else
		return -EINVAL;
	card = snd_card_get_index(name);
	if (card < 0)
		return card;
	*cardp = card;
	if (devp) {
		name = strchr(name, ',');
		if (name)
			sscanf(name, "%d,%d",  devp, subdevp);
	}
	return 0;
}
