#ifndef __LOCAL_H_INC
#define __LOCAL_H_INC

#define DEVPATH		"/dev/snd"

int _snd_dev_get_device(const char *name, int *cardp, int *devp, int *subdevp);

#endif /* __LOCAL_H_INC */
