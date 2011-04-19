/*
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

typedef FILE snd_output_t;

typedef enum _snd_output_type {
	SND_OUTPUT_STDIO,
	/* SND_OUTPUT_BUFFER */
} snd_output_type_t;

int snd_output_stdio_open(snd_output_t **outputp, const char *file, const char *mode)
{
	if ((*outputp = fopen(file, mode)) == NULL)
		return -errno;
	return 0;
}

int snd_output_stdio_attach(snd_output_t **outputp, FILE *fp, int _close)
{
	*outputp = fp;
	return 0;
}

int snd_output_close(snd_output_t *out)
{
	return fclose(out);
}

int snd_output_printf(snd_output_t *out, const char *fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = vfprintf(out, fmt, ap);
	va_end(ap);
	return ret;
}

int snd_output_puts(snd_output_t *out, const char *str)
{
	return fputs(str, out);
}

int snd_output_putc(snd_output_t *out, int c)
{
	return putc(c, out);
}

int snd_output_flush(snd_output_t *out)
{
	return fflush(out);
}

int snd_output_buffer_open(snd_output_t **outputp)
{
	return -ENXIO;
}

size_t snd_output_buffer_string(snd_output_t *output, char **buf)
{
	return 0;
}
