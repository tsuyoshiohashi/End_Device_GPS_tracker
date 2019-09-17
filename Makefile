# end device
# GPS version
PROG := enddevice
SRCS := enddevice.c aes.c frame.c radio.c timer.c appdata.c cayenne_lpp.c
HDRS := enddevice.h frame.h radio.h timer.h config.h appdata.h cayenne_lpp.h
OBJS := $(SRCS:%.c=%.o)

CC=gcc
CFLAGS= -Wall
LIBS=-lwiringPi -lgps -lm

.PHONY: all
all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)   
	
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm *.o *.gch $(PROG)
	