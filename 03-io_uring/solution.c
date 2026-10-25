#include <solution.h>
#include <liburing.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


const int URING_ENTRIES = 4;
const int BUFFER_SIZE = 1024 * 256;

int copy(int in, int out)
{
	(void) in;
	(void) out;

	/* implement me */
	int processing_reads = 0;
	int processing_writes = 0;
	int read_blocks_processed = 0;
	int write_blocks_processed = 0;

	char **buffers = malloc(URING_ENTRIES * sizeof(*buffers));
	if (buffers == NULL)
	{
		report_error("malloc error:", errno);
		return -1;
	}
	for (size_t i = 0; i < URING_ENTRIES; i++)
	{
		buffers[i] = malloc(BUFFER_SIZE);
	
		if (buffers[i] == NULL)
		{
			for (size_t j = 0; j < i; j++)
			{
				free(buffers[j]);
			}
			free(buffers);
			report_error("malloc error:", errno);
			return -1;
		}
	}

	struct io_uring ring;
	if (io_uring_queue_init(4, &ring, 0) != 0 )
	{
		report_error("io_uring_queue_init error:", errno);
		return -1;
	}
	struct io_uring_sqe* sqe;
	struct io_uring_cqe cqe;
	for (processing_reads; processing_reads < URING_ENTRIES; processing_reads++)
	{
		sqe = io_uring_get_sqe(&ring);
		if (&sqe == NULL)
		{
			report_error("io_uring_get_sqe", errno);
			return -1;
		}
		io_uring_prep_readv(sqe, in, buffers[processing_reads], 1, read_blocks_processed * BUFFER_SIZE);
	}

	// continue from here need to wait and scheduling writes and reads until all data is copied
	return 0;
}

void report_error(const char *reason, int errno_code)
{
	fprintf(stderr, "failed to access '%s': %i (%s)\n", reason, errno_code, strerror(errno_code));
}