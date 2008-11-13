all: generate_filelist sreadahead


generate_filelist: readahead.h filelist.c Makefile
	gcc -Os -g -Wall -W filelist.c -o generate_filelist

sreadahead: readahead.h readahead.c Makefile
	gcc -Os -g -Wall -lpthread -W readahead.c -o sreadahead
	
clean:
	rm -f *~ sreadahead generate_filelist