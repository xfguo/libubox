CC?=gcc
CFLAGS?=-O2
CFLAGS+=-std=gnu99 -Wall -Werror -pedantic -fpic
LDFLAGS?=
LIBNL=-lnl-tiny
PREFIX=/usr
INCLUDE_DIR=$(PREFIX)/include/libubox
LIBDIR=$(PREFIX)/lib
CPPFLAGS=

all: libubox.so

libubox.so: ucix.c blob.c blobmsg.c hash.c uhtbl.c usock.c uloop.c unl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -shared -Wl,-soname,libubox.so $^ $(LDFLAGS) -luci $(LIBNL)

install-headers:
	mkdir -p $(INCLUDE_DIR)
	cp *.h $(INCLUDE_DIR)/

install-lib:
	mkdir -p $(LIBDIR)
	cp libubox.so $(LIBDIR)/

install: install-lib install-headers

clean:
	rm -f *.so

