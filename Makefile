CFLAGS?=-Os -march=i686 -g -Wall
PROGS=sreadahead-pack sreadahead

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
