
LIBUSB = 1
CFLAGS = -O2 -Wall -Wextra -std=c99 -pedantic -Wno-unused
CFLAGS += -DUSE_LIBUSB=$(LIBUSB)
LIBS = -lm -lpthread
APPNAME = spd_platformer

ifeq ($(LIBUSB), 1)
LIBS += -lusb-1.0
endif

$(APPNAME): $(APPNAME).c common.c
	$(CC) -s $(CFLAGS) -o $@ $^ $(LIBS)
