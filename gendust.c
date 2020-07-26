#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#include <errno.h>
#include <libdevmapper.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/random.h>

//
// libdevmapper's api use 0 to indicate failure.
//

static void
add_bad_blocks(uint64_t bblock, const char *dmname, uint64_t sector)
{
	struct dm_task *dmt = NULL;
	char *msgstr = NULL;
	uint64_t i;

	dmt = dm_task_create(DM_DEVICE_TARGET_MSG);
	if (dmt == NULL) {
		fprintf(stderr, "dm_task_create failed for %s\n", __func__);
		goto errout;
	}

	if (!dm_task_set_name(dmt, dmname)) {
		fprintf(stderr, "dm_task_set_name failed with %d\n", dm_task_get_errno(dmt));
		goto errout;
	}

	if (!dm_task_set_sector(dmt, sector)) {
		fprintf(stderr, "dm_task_set_sector failed with %d\n", dm_task_get_errno(dmt));
		goto errout;
	}

	asprintf(&msgstr, "addbadblock %" PRIu64, bblock);
	if (!dm_task_set_message(dmt, msgstr))
		goto errout;

	if (!dm_task_run(dmt)) {
		fprintf(stderr, "dm_task_run failed with %d\n", dm_task_get_errno(dmt));
		goto errout;
	}

	free(msgstr);
	dm_task_destroy(dmt);
	return;

errout:
	free(msgstr);
	if (dmt)
		dm_task_destroy(dmt);
	return;
}

static void
gen_bad_blocks(uint64_t device_size, uint64_t nr_bblock, uint64_t start, const char *dmname, uint64_t sector, FILE *fout)
{
	uint64_t i;
	uint64_t offset = start;

	for (i = 0; i < nr_bblock; i++) {
		uint64_t randu64;
		uint64_t block = offset;
		if (getrandom(&randu64, sizeof(randu64), 0) == -1)
			fprintf(stderr, "Warning: the %" PRIu64 "th block may not be random\n", i);
		block += randu64 % (device_size - offset);
		add_bad_blocks(block, dmname, sector);
		if (fout)
			fprintf(fout, "%" PRIu64 "\n", block);
	}
}

//
// Usage: gendust dmname number_of_bad_blocks
//
int
main(int argc, char **argv)
{
	struct dm_task *dmt = NULL;
	struct dm_info dmi;
	void *dmnext = NULL;
	int exitcode = EXIT_FAILURE;
	const char *dmname = NULL;
	unsigned long long nr_bblocks = 0;

	if (argc != 3)
		return EXIT_FAILURE;

	errno = 0;
	nr_bblocks = strtoull(argv[3], NULL, 0);
	if (errno) {
		fprintf(stderr, "Failed to parse number_of_bad_blocks\n");
		return EXIT_FAILURE;
	}

	dmt = dm_task_create(DM_DEVICE_STATUS);
	if (dmt == NULL) {
		fprintf(stderr, "dm_task_create failed\n");
		return EXIT_FAILURE;
	}

	if (!dm_task_set_name(dmt, argv[1])) {
		fprintf(stderr, "dm_task_set_name failed with %d\n", dm_task_get_errno(dmt));
		goto errout;
	}

	if (!dm_task_run(dmt)) {
		fprintf(stderr, "dm_task_run failed with %d\n", dm_task_get_errno(dmt));
		goto errout;
	}

	if (!dm_task_get_info(dmt, &dmi)) {
		fprintf(stderr, "dm_task_get_info failed with %d\n", dm_task_get_errno(dmt));
		goto errout;
	}
	if (!dmi.exists) {
		fprintf(stderr, "Target %s does not exist\n", argv[1]);
		goto errout;
	}

	dmname = dm_task_get_name(dmt);

	do {
		uint64_t start, length;
		char *target_type, *params;

		dmnext = dm_get_next_target(dmt, dmnext, &start, &length, &target_type, &params);
		// Start from 1 to avoid manual intervention of selecting backup superblock
		if (!strcmp(target_type, "dust"))
			gen_bad_blocks(length, nr_bblocks, 1, dmname, 0, stdout);
	} while (dmnext);

	dm_task_destroy(dmt);
	return EXIT_SUCCESS;

errout:
	dm_task_destroy(dmt);
	return exitcode;
}
