TARGETS = wc_server viewer 

CC = gcc
CFLAGS = -c -g -O2 -pthread -fsigned-char -Wall 
SDLCFLAGS = $(shell sdl2-config --cflags)

WC_SERVER_OBJS = wc_main.o wc_webcam.o net.c util.o jpeg_decode.o temper.o
VIEWER_OBJS    = viewer.o net.c util.o jpeg_decode.o

#
# build rules
#

all: $(TARGETS)

wc_server: $(WC_SERVER_OBJS) 
	$(CC) -pthread -lrt -ljpeg -lreadline -lusb -lm -o $@ $(WC_SERVER_OBJS)

viewer: $(VIEWER_OBJS) 
	$(CC) -pthread -lrt -ljpeg -lSDL2 -lSDL2_ttf -lSDL2_mixer -lreadline -o $@ $(VIEWER_OBJS)

#
# clean rule
#

clean:
	rm -f $(TARGETS) *.o

#
# compile rules
#

wc_main.o:       wc_main.c wc.h
wc_webcam.o:     wc_webcam.c wc.h
util.o:          util.c wc.h
jpeg_decode.o:   jpeg_decode.c wc.h
temper.o:        temper.c wc.h
net.o:           net.c wc.h

viewer.o: viewer.c wc.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) $< -o $@

.c.o: 
	$(CC) $(CFLAGS) $< -o $@
