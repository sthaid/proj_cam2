TARGETS = viewer wc_server

CC = gcc
OUTPUT_OPTION=-MMD -MP -o $@
CFLAGS = -Wall -g -O2 -I. -fsigned-char

viewer.o: CFLAGS += $(shell sdl2-config --cflags)

SRC_VIEWER    = viewer.c net.c util.c jpeg_decode.c
SRC_WC_SERVER = wc_main.c wc_webcam.c net.c util.c jpeg_decode.c temper.c

DEP = $(SRC_VIEWER:.c=.d) $(SRC_WC_SERVER:.c=.d)

#
# build rules
#

all: $(TARGETS)

viewer: $(SRC_VIEWER:.c=.o)
	$(CC) -lpthread -ljpeg -lSDL2 -lSDL2_ttf -lSDL2_mixer \
              -o $@ $(SRC_VIEWER:.c=.o)

wc_server: $(SRC_WC_SERVER:.c=.o)
	$(CC) -lpthread -ljpeg -lusb -lm -lrt \
              -o $@ $(SRC_WC_SERVER:.c=.o)

-include $(DEP)

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(DEP) $(DEP:.d=.o)
