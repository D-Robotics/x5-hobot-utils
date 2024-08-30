/*
 * Copyright (C) 2000, Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 * Copyright (C) 2008, BusyBox Team. -solar 4/26/08
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
//config:config DEVMEM
//config:	bool "devmem (2.5 kb)"
//config:	default y
//config:	help
//config:	devmem is a small program that reads and writes from physical
//config:	memory using /dev/mem.

//applet:IF_DEVMEM(APPLET(devmem, BB_DIR_SBIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_DEVMEM) += devmem.o

//usage:#define devmem_trivial_usage
//usage:	"ADDRESS [WIDTH [VALUE]]"
//usage:#define devmem_full_usage "\n\n"
//usage:       "Read/write from physical address\n"
//usage:     "\n	ADDRESS	Address to act upon"
//usage:     "\n	WIDTH	Width (8/16/...)"
//usage:     "\n	VALUE	Data to be written"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>

void show_usage(void)
{
	printf("Usage: devmem ADDRESS [WIDTH [VALUE]]\n");
	printf("\nRead/write from physical address\n");
	printf("\n\tADDRESS Address to act upon\n");
	printf("\tWIDTH   Width (8/16/...)\n");
	printf("\tVALUE   Data to be written\n");
	exit(0);
}

static unsigned long long ret_ERANGE(void)
{   
    errno = ERANGE; /* this ain't as small as it looks (on glibc) */
    return ULLONG_MAX;
}

static unsigned long long handle_errors(unsigned long long v, char **endp)
{   
    char next_ch = **endp;
       
    /* errno is already set to ERANGE by strtoXXX if value overflowed */
    if (next_ch) {
        /* "1234abcg" or out-of-range? */
        if (isalnum(next_ch) || errno)
            return ret_ERANGE();
        /* good number, just suspicious terminator */
        errno = EINVAL;
    }  
    return v;
}

unsigned long long bb_strtoull(const char *arg, char **endp, int base)
{
    unsigned long long v;
    char *endptr;
 
    if (!endp) endp = &endptr;
    *endp = (char*) arg;
 
    /* strtoul("  -4200000000") returns 94967296, errno 0 (!) */
    /* I don't think that this is right. Preventing this... */
    if (!isalnum(arg[0])) return ret_ERANGE();
 
    /* not 100% correct for lib func, but convenient for the caller */
    errno = 0;
    v = strtoull(arg, endp, base);
    return handle_errors(v, endp);
}

int main(int argc, char **argv)
{
	void *map_base, *virt_addr;
	uint64_t read_result;
	uint64_t writeval = writeval; /* for compiler */
	off_t target;
	unsigned page_size, mapped_size, offset_in_page;
	int fd;
	unsigned width = 8 * sizeof(int);

	/* devmem ADDRESS [WIDTH [VALUE]] */
// TODO: options?
// -r: read and output only the value in hex, with 0x prefix
// -w: write only, no reads before or after, and no output
// or make this behavior default?
// Let's try this and see how users react.

	/* ADDRESS */
	if (!argv[1])
		show_usage();
	errno = 0;
	target = bb_strtoull(argv[1], NULL, 0); /* allows hex, oct etc */

	/* WIDTH */
	if (argv[2]) {
		if (isdigit(argv[2][0]) || argv[2][1])
			width = atoll(argv[2]);
		else {
			static const char bhwl[] = "bhwl";
			static const uint8_t sizes[] = {
				8 * sizeof(char),
				8 * sizeof(short),
				8 * sizeof(int),
				8 * sizeof(long),
				0 /* bad */
			};
			width = strchr
			(bhwl, (argv[2][0] | 0x20)) - bhwl;
			width = sizes[width];
		}
		/* VALUE */
		if (argv[3])
			writeval = bb_strtoull(argv[3], NULL, 0);
	} else { /* argv[2] == NULL */
		/* make argv[3] to be a valid thing to fetch */
		argv--;
	}
	if (errno)
		show_usage(); /* one of bb_strtouXX failed */

	fd = open("/dev/mem", argv[3] ? (O_RDWR | O_SYNC) : (O_RDONLY | O_SYNC));
	mapped_size = page_size = getpagesize();
	offset_in_page = (unsigned)target & (page_size - 1);
	if (offset_in_page + width > page_size) {
		/* This access spans pages.
		 * Must map two pages to make it possible: */
		mapped_size *= 2;
	}
	map_base = mmap(NULL,
			mapped_size,
			argv[3] ? (PROT_READ | PROT_WRITE) : PROT_READ,
			MAP_SHARED,
			fd,
			target & ~(off_t)(page_size - 1));
	if (map_base == MAP_FAILED) {
		printf("mmap failed\n");
		exit(-1);
	}

//	printf("Memory mapped at address %p.\n", map_base);

	virt_addr = (char*)map_base + offset_in_page;

	if (!argv[3]) {
		switch (width) {
		case 8:
			read_result = *(volatile uint8_t*)virt_addr;
			break;
		case 16:
			read_result = *(volatile uint16_t*)virt_addr;
			break;
		case 32:
			read_result = *(volatile uint32_t*)virt_addr;
			break;
		case 64:
			read_result = *(volatile uint64_t*)virt_addr;
			break;
		default:
			printf("bad width\n");
			exit(-1);
		}
//		printf("Value at address 0x%"OFF_FMT"X (%p): 0x%llX\n",
//			target, virt_addr,
//			(unsigned long long)read_result);
		/* Zero-padded output shows the width of access just done */
		printf("0x%0*llX\n", (width >> 2), (unsigned long long)read_result);
	} else {
		switch (width) {
		case 8:
			*(volatile uint8_t*)virt_addr = writeval;
//			read_result = *(volatile uint8_t*)virt_addr;
			break;
		case 16:
			*(volatile uint16_t*)virt_addr = writeval;
//			read_result = *(volatile uint16_t*)virt_addr;
			break;
		case 32:
			*(volatile uint32_t*)virt_addr = writeval;
//			read_result = *(volatile uint32_t*)virt_addr;
			break;
		case 64:
			*(volatile uint64_t*)virt_addr = writeval;
//			read_result = *(volatile uint64_t*)virt_addr;
			break;
		default:
			printf("bad width\n");
			exit(-1);
		}
//		printf("Written 0x%llX; readback 0x%llX\n",
//				(unsigned long long)writeval,
//				(unsigned long long)read_result);
	}

	munmap(map_base, mapped_size);
	close(fd);

	return 0;
}
