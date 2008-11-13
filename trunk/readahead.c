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
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "readahead.h"

#define MAXR 1024
static struct readahead files[MAXR];
static unsigned int total_files = 0;

static unsigned int cursor = 0;
void readahead_one(int index)
{
	int fd;
	int i;

	fd = open(files[index].filename, O_RDONLY|O_NOATIME);
	if (fd<1)
		fd = open(files[index].filename, O_RDONLY);
	if (fd<1) {
		printf("returned %i, errno is %i \n", fd, errno);
		return;
	}
	for (i = 0; i < 6; i++) {
		if (files[index].data[i].len)
			readahead(fd, files[index].data[i].offset, files[index].data[i].len);
	}
	close(fd);
}

void *one_thread(void *ptr)
{
	while (1) {
		unsigned int mine;

		mine = __sync_fetch_and_add(&cursor, 1);
		if (mine < total_files)
			readahead_one(mine);
		else
			break;
	}
	return NULL;
}


int main(int argc, char **argv)
{
	FILE *file = fopen("/etc/readahead.packed", "r");
	
	daemon(0,0);
	
	total_files = fread(&files, sizeof(struct readahead), MAXR, file);
	
	pthread_t one, two, three, four;

	pthread_create(&one, NULL, one_thread, NULL);
	pthread_create(&two, NULL, one_thread, NULL);
	pthread_create(&three, NULL, one_thread, NULL);
	pthread_create(&four, NULL, one_thread, NULL);

	printf("Waiting\n");
	pthread_join(one, NULL);
	pthread_join(two, NULL);
	pthread_join(three, NULL);
	pthread_join(four, NULL);

	fclose(file);
	return EXIT_SUCCESS;
}
