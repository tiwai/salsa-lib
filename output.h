#ifndef __ALSA_OUTPUT_H
#define __ALSA_OUTPUT_H

#include <stdio.h>

typedef FILE snd_output_t;

typedef enum _snd_output_type {
	SND_OUTPUT_STDIO,
	/* SND_OUTPUT_BUFFER */
} snd_output_type_t;

static inline
int snd_output_stdio_open(snd_output_t **outputp, const char *file, const char *mode)
{
	if ((*outputp = fopen(file, mode)) == NULL)
		return -errno;
	return 0;

}

static inline
int snd_output_stdio_attach(snd_output_t **outputp, FILE *fp, int _close)
{
	*outputp = fp;
}

#define snd_output_close(out)			fclose(out)
#define snd_output_printf(out, format...)	vfprintf(out, format)
#define snd_output_puts(out, str)		fputs(str, out)
#define snd_output_putc(out, c)			putc(c, out)
#define snd_output_flush(out)			fflush(out)

static inline
int snd_output_buffer_open(snd_output_t **outputp)
{
	return -ENODEV;
}

static inline
size_t snd_output_buffer_string(snd_output_t *output, char **buf)
{
	return 0;
}

#endif /* __ALSA_OUTPUT_H */
