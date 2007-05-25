static inline
const char *snd_strerror(int errnum)
{
	if (errnum < 0)
		errnum = -errnum;
	return (const char *) strerror(errnum);
}

static inline
int snd_lib_error_set_handler(snd_lib_error_handler_t handler)
{
	return 0;
}

static inline
const char *snd_asoundlib_version(void)
{
	return SND_LIB_VERSION_STR;
}
