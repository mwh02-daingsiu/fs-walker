#define _FILE_OFFSET_BITS 64

#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <sys/random.h>
#include <set>

static unsigned int block_size = 4096;

static void
gen_bad_blocks(uint64_t fs_blocks, uint64_t nr_bblock, uint64_t start_block, FILE *fout)
{
	uint64_t i;
	uint64_t randu64;
	uint64_t offset = start_block;
	std::set<uint64_t> dedup_table;

	if (getrandom(&randu64, sizeof(randu64), 0) == -1)
		fprintf(stderr, "Warning: the %" PRIu64 "th block may not be random\n", i);

	srandom(randu64);

	for (i = 0; i < nr_bblock; i++) {
		uint64_t block;
		do {
			block = offset;
			block += random() % (fs_blocks - offset);
			if (dedup_table.find(block) == dedup_table.end()) {
				dedup_table.emplace(block);
				break;
			}
		} while (1);
		if (fout)
			fprintf(fout, "%" PRIu64 "\n", block);
	}
}

//
// Usage: gendust device_size number_of_bad_blocks start_block
//
int
main(int argc, char **argv)
{
	unsigned long long device_size = 0;
	unsigned long long nr_bblocks = 0;
	unsigned long long start_block = 0;
	unsigned long long fs_blocks = 0;
	const char *bsizestr;

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
	start_block = strtoull(argv[3], NULL, 0);
	if (errno) {
		fprintf(stderr, "Failed to parse start_block\n");
		return EXIT_FAILURE;
	}

	bsizestr = getenv("BLOCK_SIZE");
	if (bsizestr) {
		unsigned long v;

		errno = 0;
		v = strtoul(bsizestr, NULL, 0);
		if (errno) {
			fprintf(stderr, "Failed to parse BLOCK_SIZE\n");
			return EXIT_FAILURE;
		}
		block_size = v;
	}

	fs_blocks = device_size / (block_size / 512);

	gen_bad_blocks(fs_blocks, nr_bblocks, start_block, stdout);
	return EXIT_SUCCESS;
}
