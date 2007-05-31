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
	int open_dev;
	char control[sizeof(SND_FILE_CONTROL) + 10];

	sprintf(control, SND_FILE_CONTROL, card);

	open_dev = open(control, O_RDONLY);
	if (open_dev >= 0) {
		close(open_dev);
		return 1;
	}
	return 0;
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

static int snd_ctl_hw_open(snd_ctl_t **ctlp, int card)
{
	char name[16];
	sprintf(name, "hw:%d", card);
	return snd_ctl_open(ctlp, name, 0);
}

int snd_card_get_index(const char *string)
{
	int card;
	snd_ctl_t *handle;
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
		if (! snd_card_load(card))
			continue;
		if (snd_ctl_hw_open(&handle, card) < 0)
			continue;
		if (snd_ctl_card_info(handle, &info) < 0) {
			snd_ctl_close(handle);
			continue;
		}
		snd_ctl_close(handle);
		if (!strcmp((const char *)info.id, string))
			return card;
	}
	return -ENODEV;
}

int snd_card_get_name(int card, char **name)
{
	snd_ctl_t *handle;
	snd_ctl_card_info_t info;
	int err;
	
	if (name == NULL)
		return -EINVAL;
	if ((err = snd_ctl_hw_open(&handle, card)) < 0)
		return err;
	if ((err = snd_ctl_card_info(handle, &info)) < 0) {
		snd_ctl_close(handle);
		return err;
	}
	snd_ctl_close(handle);
	*name = strdup((const char *)info.name);
	if (*name == NULL)
		return -ENOMEM;
	return 0;
}

int snd_card_get_longname(int card, char **name)
{
	snd_ctl_t *handle;
	snd_ctl_card_info_t info;
	int err;
	
	if (name == NULL)
		return -EINVAL;
	if ((err = snd_ctl_hw_open(&handle, card)) < 0)
		return err;
	if ((err = snd_ctl_card_info(handle, &info)) < 0) {
		snd_ctl_close(handle);
		return err;
	}
	snd_ctl_close(handle);
	*name = strdup((const char *)info.longname);
	if (*name == NULL)
		return -ENOMEM;
	return 0;
}
