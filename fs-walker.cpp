#define _FILE_OFFSET_BITS 64

#include <fts.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static unsigned long long status_completed_directory = 0;
static unsigned long long status_completed_file = 0;
static unsigned long long status_error_directory = 0;
static unsigned long long status_error_file = 0;
static unsigned long long status_partial_directory = 0;
static unsigned long long status_partial_file = 0;
static unsigned long long status_undefined_error = 0;

static const unsigned long max_legal_readunit = 65536;

static void
visit_file(const char *filename, struct stat *statbuf)
{
	off_t filesize = statbuf->st_size;
	unsigned long readunit = statbuf->st_blksize;
	std::vector<char> buf(readunit);
	off_t rem = filesize;
	int fd = -1;

	if (readunit == 0 || readunit > max_legal_readunit) {
		goto error_file;
	}

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		goto error_file;
	}

	while (rem > 0) {
		ssize_t r = read(fd, &buf[0], (rem < readunit) ? rem : readunit);
		if (r < 0) {
			if (rem == filesize)
				goto error_file;
			goto partial_file;
		}

		rem -= r;
	}

ok_file:
	status_completed_file++;
	close(fd);
	return;

error_file:
	status_error_file++;
	if (fd != -1)
		close(fd);
	return;

partial_file:
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
}

int
main(int argc, char **argv)
{
	FTS *fts;
	FTSENT *ent;

	if (argc < 2)
		return EXIT_FAILURE;

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
	}

	fts_close(fts);

	print_stat();

	return EXIT_SUCCESS;
}
