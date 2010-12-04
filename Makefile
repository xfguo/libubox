CC?=gcc
CFLAGS?=-O2
CFLAGS+=-std=gnu99 -Wall -Werror -pedantic -fpic
LDFLAGS?=
LIBNL=-lnl-tiny
PREFIX=/usr
INCLUDE_DIR=$(PREFIX)/include/libubox
LIBDIR=$(PREFIX)/lib
CPPFLAGS=

OS=$(shell uname)
FILES=blob.c blobmsg.c hash.c uhtbl.c usock.c
ifeq ($(OS),Linux)
  FILES += uloop.c unl.c
  LIBS += $(LIBNL)
  LDFLAGS_SHARED=-shared -Wl,-soname,$@
  SHLIB_EXT=so
endif
ifeq ($(OS),Darwin)
  LDFLAGS_SHARED=-dynamiclib
  SHLIB_EXT=dylib
endif

all: libubox.$(SHLIB_EXT)

libubox.$(SHLIB_EXT): $(FILES)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS) $(LDFLAGS_SHARED)

install-headers:
	mkdir -p $(INCLUDE_DIR)
	cp *.h $(INCLUDE_DIR)/

install-lib:
	mkdir -p $(LIBDIR)
	cp libubox.$(SHLIB_EXT) $(LIBDIR)/

install: install-lib install-headers

clean:
	rm -f *.$(SHLIB_EXT)

