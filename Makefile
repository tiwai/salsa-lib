OBJS = control.o pcm.o pcm_params.o pcm_misc.o hcontrol.o mixer.o

CFLAGS = -Wall -g -O2

all: $(OBJS)

clean:
	rm -f $(OBJS)
