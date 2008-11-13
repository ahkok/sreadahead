#ifndef __INCLUDE_GUARD_READAHEAD_H_
#define __INCLUDE_GUARD_READAHEAD_H_


#include <stdint.h>

struct record {
	uint32_t	offset;
	uint32_t	len;
};

struct readahead {
	char		filename[100];
	unsigned long	jiffies;
	struct record	data[6];
};

#endif

