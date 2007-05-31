#ifndef __ALSA_INPUT_H
#define __ALSA_INPUT_H

#include <stdio.h>
#include <errno.h>

typedef FILE snd_input_t;

/** Input type. */
typedef enum _snd_input_type {
	/** Input from a stdio stream. */
	SND_INPUT_STDIO,
} snd_input_type_t;

static inline
int snd_input_stdio_open(snd_input_t **inputp, const char *file, const char *mode)
{
	if ((*inputp = fopen(file, mode)) == NULL)
		return -errno;
	return 0;
}

static inline
int snd_input_stdio_attach(snd_input_t **inputp, FILE *fp, int _close)
{
	*inputp = fp;
	return 0;
}
	
#define snd_input_close(input)		fclose(input)
#define snd_input_scanf			fscanf
#define snd_input_gets(input,str,size)	fgets(str, size, input)
#define snd_input_getc(input)		getc(input)
#define snd_input_ungetc(input,c)	ungetc(c, input)

static inline
int snd_input_buffer_open(snd_input_t **inputp, const char *buffer, ssize_t size)
{
	return -ENODEV;
}

#endif /* __ALSA_INPUT_H */
