/*
 * (C) Copyright 2008 Intel Corporation
 *
 * Author: Arjan van de Ven <arjan@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include <getopt.h>

#define EXT3_IOC_INODE_JIFFIES          _IOR('f', 19, long)

#include "readahead.h"

static struct option opts[] = {
	{ "debug", 0, NULL, 'd' },
	{ 0, 0, NULL, 0 }
};

int debug = 0;

struct readahead RA[4096];
int RAcount;

FILE *output;

static int smallest_gap(struct record *record, int count)
{
	int i;
	int cur = 0, maxgap;

	maxgap = 1024*1024*512;
	
	for (i = 0; i < count; i++, record++) {
		if (i+1 < count) {
			int gap;
			gap = (record+1)->offset - record->offset - record->len;
			if (gap < maxgap) {
				maxgap = gap;
				cur = i;
			}
		}
	}
	return cur;
}

void dump_records(struct record *record, int count)
{
	int i;
	int q;
	q = smallest_gap(record, count);
	
	for (i = 0; i < count; i++, record++) {
		printf("Record %i:  Start at %iKb for %iKb", i,
		       record->offset/1024, (record->len+1)/1024);
		if (i+1 < count) {
			int gap;
			gap = (record+1)->offset - record->offset - record->len;
			printf("\t gap is %iKb", gap/1024);
		}
		if (q == i) printf("*");
		printf("\n");
	}
}

static int merge_record(struct record *record, int count, int to_merge)
{
	record[to_merge].len = record[to_merge+1].offset+record[to_merge+1].len - record[to_merge].offset;
	memcpy(&record[to_merge+1], &record[to_merge+2], sizeof(struct record) * (count-to_merge-2));
	return count - 1;
}

static int reduce_to(struct record *record, int count, int target)
{
	while (count > target) {
		int tomerge;
		tomerge = smallest_gap(record, count);
		count = merge_record(record, count, tomerge);
	}
	return count;
}

int do_file(char *filename)
{
	FILE *file;
	struct stat statbuf;
	void *mmapptr;
	unsigned char *mincorebuf;
	int fd;
	int i;

	struct record record[4096];
	int reccount = 0;

	int phase;
	uint32_t start;
	int there=0, notthere=0;
	unsigned long time;

	memset(record, 0, sizeof(record));

	file = fopen(filename, "r");
	if (!file)
		return 0;
	fd = fileno(file);
	
	time = ioctl(fd, EXT3_IOC_INODE_JIFFIES);
	time += 300000;

	fstat(fd, &statbuf);
	mmapptr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	
	mincorebuf = malloc(statbuf.st_size/4096 + 1);
	mincore(mmapptr, statbuf.st_size, mincorebuf);
	
	if (mincorebuf[0]) {
		phase = 1;
		start = 0;
	} else {
		phase = 0;
	}

	for (i = 0; i <= statbuf.st_size; i+=4096) {
		if (mincorebuf[i/4096])
			there++;
		else
			notthere++;
		if (phase == 1 && !mincorebuf[i/4096]) {
			phase = 0;
			if (i>statbuf.st_size)
				i = statbuf.st_size+1;
			record[reccount].offset = start;
			record[reccount].len = i - 1 - start;
			reccount++;
			if (reccount >= 4000) reccount = 4000;
		} else if (phase == 0 && mincorebuf[i/4096]) {
			phase = 1;
			start = i;
		}
	}

	if (phase == 1) {
		if (i>statbuf.st_size)
			i = statbuf.st_size+1;
		record[reccount].offset = start;
		record[reccount].len = i - 1 - start;
		reccount++;
	}
	free(mincorebuf);
	munmap(mmapptr, statbuf.st_size);
	fclose(file);
	
	if (reccount && debug)
		printf("File %s is %3.1f%% in memory in %i fragments \n", filename, 100.0*there/(there+notthere), reccount);

	reccount = reduce_to(record, reccount, 6);

	if (reccount>0) {
		memset(&RA[RAcount], 0, sizeof(struct readahead));
		strcpy(RA[RAcount].filename, filename);
		memcpy(RA[RAcount].data, record, sizeof(RA[RAcount].data));
		RA[RAcount].jiffies = time;
		RAcount++;
		return 1;
	}
	return 0;
}

void do_metafile(const char *filename)
{
	FILE *file;
	char line[4096];
	char *c;
	int count = 0;
	int lines = 0;

	file = fopen(filename, "r");
	if (!file)
		return;
	while (fgets(line, 4095, file) != NULL) {
		lines++;
		if (strlen(line)<4)
			break;
		c = strchr(line, '\n');
		if (c) *c = 0;
		else {
			printf("Warning: file name too long: %s\n", line);
			continue; /* skip it */
		}

		count += do_file(line);
	}
	fclose(file);
	if (debug) {
		printf("Wrote %i out of %i lines\n", count, lines);
		printf("RAcount is %i \n", RAcount);
	}
}

void sort_RA(void)
{
	int delta = 1;
	
	while (delta>0) {
		int i;
		delta = 0;
		for (i=0; i < RAcount-1; i++) 
			if (RA[i].jiffies > RA[i+1].jiffies) {
				struct readahead tmp;
				tmp = RA[i+1];
				RA[i+1] = RA[i];
				RA[i] = tmp;
				delta++;
			}
	}
}

int main(int argc, char **argv)
{
	const char *out = "readahead.packed";

	while (1) {
		int index = 0, c;
		c = getopt_long(argc, argv, "d", opts, &index);
		if (c == -1)
			break;
		switch (c) {
		case 'd':
			debug = 1;
			break;
		default:
			;
		}
	}

	do_metafile(argv[optind]);
	sort_RA();

	output = fopen(out, "w");
	if (!output) {
		perror(out);
		return 1;
	}
	fwrite(&RA, RAcount, sizeof(struct readahead), output);
	fclose(output);

	return EXIT_SUCCESS;
}
