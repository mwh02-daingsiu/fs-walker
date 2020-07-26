#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/random.h>

static void
gen_bad_blocks(uint64_t device_size, uint64_t nr_bblock, uint64_t start, FILE *fout)
{
	uint64_t i;
	uint64_t offset = start;

	for (i = 0; i < nr_bblock; i++) {
		uint64_t randu64;
		uint64_t block = offset;
		if (getrandom(&randu64, sizeof(randu64), 0) == -1)
			fprintf(stderr, "Warning: the %" PRIu64 "th block may not be random\n", i);
		block += randu64 % (device_size - offset);
		if (fout)
			fprintf(fout, "%" PRIu64 "\n", block);
	}
}

//
// Usage: gendust device_size number_of_bad_blocks start_offset
//
int
main(int argc, char **argv)
{
	unsigned long long device_size = 0;
	unsigned long long nr_bblocks = 0;
	unsigned long long start = 0;

	if (argc != 4)
		return EXIT_FAILURE;

	errno = 0;
	device_size = strtoull(argv[1], NULL, 0);
	if (errno) {
		fprintf(stderr, "Failed to parse device_size\n");
		return EXIT_FAILURE;
	}
	nr_bblocks = strtoull(argv[2], NULL, 0);
	if (errno) {
		fprintf(stderr, "Failed to parse number_of_bad_blocks\n");
		return EXIT_FAILURE;
	}
	start = strtoull(argv[3], NULL, 0);
	if (errno) {
		fprintf(stderr, "Failed to parse start_offset\n");
		return EXIT_FAILURE;
	}

	gen_bad_blocks(device_size, nr_bblocks, start, stdout);
	return EXIT_SUCCESS;
}
