#define _FILE_OFFSET_BITS 64

#include <fts.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned long long status_completed_directory = 0;
static unsigned long long status_completed_file = 0;
static unsigned long long status_error_directory = 0;
static unsigned long long status_error_file = 0;
static unsigned long long status_partial_directory = 0;
static unsigned long long status_partial_file = 0;
static unsigned long long status_undefined_error = 0;

static unsigned long long status_all_file_total_chunks = 0;
static unsigned long long status_all_file_missing_chunks = 0;
static unsigned long long status_all_file_good_chunks = 0;

static unsigned long long status_huge_file_total_chunks = 0;
static unsigned long long status_huge_file_missing_chunks = 0;
static unsigned long long status_huge_file_good_chunks = 0;

static unsigned long max_legal_readunit = 65536;
static off_t huge_file_thresh = 1ull * 1024 * 1024 * 1024;

static void
visit_file(const char *filename, struct stat *statbuf)
{
	off_t filesize = statbuf->st_size;
	unsigned long readunit = statbuf->st_blksize;
	off_t off = 0, readbytes = 0;
	int fd = -1;
	int is_huge_file = !!(filesize >= huge_file_thresh);
	int has_error = 0;
	int progress = -1;
	void *buf = NULL;

	buf = malloc(readunit);

	if (readunit == 0 || readunit > max_legal_readunit) {
		goto error_file;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		goto error_file;
	}

	while (off < filesize) {
		size_t xmit = (filesize - off < readunit) ? filesize - off : readunit;
		ssize_t r = pread(fd, buf, xmit, off);
		if (r != 0) {
			status_all_file_total_chunks++;
			if (is_huge_file)
				status_huge_file_total_chunks++;
		}
		if (r < 0) {
			status_all_file_missing_chunks++;
			if (is_huge_file)
				status_huge_file_missing_chunks++;

			has_error = 1;
		} else if (r == 0) {
			break;
		} else {
			status_all_file_good_chunks++;
			if (is_huge_file)
				status_huge_file_good_chunks++;

		}

		if (r > 0)
			readbytes += r;
		off += xmit;
		if (is_huge_file && off * 100 / filesize > progress) {
			progress = off * 100 / filesize;
			fprintf(stderr, "\rReading file %s, progress: (%5d%%)", filename, progress);
		}
	}
	if (is_huge_file)
			fprintf(stderr, "\n");

	if (has_error)
		goto error_file;

ok_file:
	free(buf);
	status_completed_file++;
	close(fd);
	return;

error_file:
	free(buf);
	if (!readbytes)
		status_error_file++;
	else
		status_partial_file++;
	if (fd != -1)
		close(fd);
}

static void
visit_dir(const char *dirname)
{
	DIR *dir = opendir(dirname);
	struct dirent *ent;
	unsigned long long n = 0;

	if (dir == NULL) {
		// Already dealt with in case of FTS_DNR
		return;
	}

	while ((ent = readdir(dir)) != NULL)
		n++;
	if (errno) {
		if (!n)
			status_error_directory++;
		else
			status_partial_directory++;
	} else
		status_completed_directory++;
	
	closedir(dir);
}

static void
print_stat()
{
	printf("status_completed_directory: %llu\n", status_completed_directory);
	printf("status_completed_file: %llu\n", status_completed_file);
	printf("status_error_directory: %llu\n", status_error_directory);
	printf("status_error_file: %llu\n", status_error_file);
	printf("status_partial_directory: %llu\n", status_partial_directory);
	printf("status_partial_file: %llu\n", status_partial_file);
	printf("status_undefined_error: %llu\n", status_undefined_error);
	printf("=================================\n");
	printf("status_all_file_total_chunks: %llu\n", status_all_file_total_chunks);
	printf("status_all_file_missing_chunks: %llu\n", status_all_file_missing_chunks);
	printf("status_all_file_total_chunks: %llu\n", status_all_file_good_chunks);
	printf("================================= (For files bigger or equal to %lld)\n", (long long)huge_file_thresh);
	printf("status_huge_file_total_chunks: %llu\n", status_huge_file_total_chunks);
	printf("status_huge_file_missing_chunks: %llu\n", status_huge_file_missing_chunks);
	printf("status_huge_file_total_chunks: %llu\n", status_huge_file_good_chunks);
}

int
main(int argc, char **argv)
{
	FTS *fts;
	FTSENT *ent;
	char *v;

	if (argc < 2)
		return EXIT_FAILURE;

	v = getenv("huge_file_thresh");
	if (v) {
		errno = 0;
		unsigned long long vi = strtoull(v, NULL, 0);
		if (!errno)
			huge_file_thresh = vi;
	}

	fts = fts_open(argv + 1, FTS_PHYSICAL | FTS_XDEV, NULL);

	while ((ent = fts_read(fts)) != NULL) {
		switch (ent->fts_info) {
		case FTS_D:
			break;
		case FTS_DC: // Error: cyclic
			status_undefined_error++;
			break;
		case FTS_DEFAULT:
			break;
		case FTS_DNR: // Error: unreadable dir
			status_error_directory++;
			break;
		case FTS_DOT:
			break;
		case FTS_DP:
			visit_dir(ent->fts_path);
			break;
		case FTS_ERR: // Error: error
			status_undefined_error++;
			break;
		case FTS_F:
			visit_file(ent->fts_path, ent->fts_statp);
			break;
		case FTS_NS:
			status_undefined_error++;
			break;
		case FTS_NSOK:
			break;
		case FTS_SL:
			break;
		case FTS_SLNONE:
			break;
		default:
			break;
		}
		if (ent->fts_path)
			fprintf(stderr, "Visited %s\n", ent->fts_path);
	}

	fts_close(fts);

	printf("=================================\n");
	print_stat();

	return EXIT_SUCCESS;
}
