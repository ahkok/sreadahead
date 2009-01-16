CFLAGS ?= -Os -march=i686 -g -Wall
PROGS = sreadahead-pack sreadahead

VERSION = 0.04

all: $(PROGS)


sreadahead-pack: readahead.h filelist.c Makefile
	gcc $(CFLAGS) -W filelist.c -o $@

sreadahead: readahead.h readahead.c Makefile
	gcc $(CFLAGS) -lpthread -W readahead.c -o $@
	
clean:
	rm -f *~ $(PROGS)

install: all
	mkdir -p $(DESTDIR)/sbin
	install -p -m 755 $(PROGS) $(DESTDIR)/sbin

dist:
	svn export . sreadahead-$(VERSION)
	tar cz --owner=root --group=root \
		-f sreadahead-$(VERSION).tar.gz sreadahead-$(VERSION)/
	rm -rf sreadahead-$(VERSION)
