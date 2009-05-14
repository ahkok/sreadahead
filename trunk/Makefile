CFLAGS ?= -Os -march=native -g
PROGS = sreadahead

VERSION = "1.0"

all: $(PROGS)

sreadahead: sreadahead.c Makefile
	gcc $(CFLAGS) -lpthread -W sreadahead.c -o $@

clean:
	rm -f *~ $(PROGS)

install: all
	mkdir -p $(DESTDIR)/sbin
	mkdir -p $(DESTDIR)/var/lib/sreadahead/debugfs
	install -p -m 755 $(PROGS) $(DESTDIR)/sbin

dist:
	svn export . sreadahead-$(VERSION)
	tar cz --owner=root --group=root \
		-f sreadahead-$(VERSION).tar.gz sreadahead-$(VERSION)/
	rm -rf sreadahead-$(VERSION)
