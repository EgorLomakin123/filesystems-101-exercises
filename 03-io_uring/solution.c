#include <solution.h>
#include <liburing.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define URING_ENTRIES 4
#define BUFFER_SIZE (1024 * 256)

void report_error(const char *reason, int errno_code)
{
	fprintf(stderr, "failed to access '%s': %i (%s)\n", reason, errno_code, strerror(errno_code));
}

int copy(int in, int out)
{
	(void) in;
	(void) out;

	/* implement me */
	int processing_reads = 0;
	int processing_writes = 0;
	long read_blocks_processed = 0;
	long buffer_offset_block_id[URING_ENTRIES] = {0};

	char **buffers = malloc(URING_ENTRIES * sizeof(*buffers));
	int buffer_state[URING_ENTRIES] = {0};
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
	struct io_uring_cqe* cqe;
	for (; processing_reads < URING_ENTRIES; processing_reads++)
	{
		sqe = io_uring_get_sqe(&ring);
		if (sqe == NULL)
		{
			report_error("io_uring_get_sqe", errno);
			return -1;
		}
		io_uring_prep_read(sqe, in,
			buffers[processing_reads], BUFFER_SIZE,
			(read_blocks_processed + processing_reads) * BUFFER_SIZE);
		io_uring_sqe_set_data(sqe, (void * )(intptr_t)(processing_reads));
		buffer_offset_block_id[processing_reads] = (read_blocks_processed + processing_reads);
		buffer_state[processing_reads] = 1;
	}
	io_uring_submit(&ring);

	// continue from here need to wait and scheduling writes and reads until all data is copied
	bool end_reads = false;
	while (processing_reads + processing_writes > 0)
	{
		io_uring_wait_cqe(&ring, &cqe);

		struct io_uring_cqe* cqes[URING_ENTRIES];
		unsigned int ended_cqes = io_uring_peek_batch_cqe(&ring, cqes, URING_ENTRIES);
		for (unsigned int i = 0; i < ended_cqes; i++)
		{
			int buffer_index = (int)(intptr_t)io_uring_cqe_get_data(cqes[i]);
			if (cqes[i]->res < 0)
			{
				report_error("reading error", errno);
				return -1;
			}
			if (buffer_state[buffer_index] == 1)
			{
				processing_reads --;
				read_blocks_processed++;

				struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
				if (sqe == NULL)
				{
					report_error("io_uring_get_sqe", errno);
					return -1;
				}

				io_uring_prep_write(sqe, out,
				 	buffers[buffer_index], cqes[i]->res,
				  	(buffer_offset_block_id[buffer_index]) * BUFFER_SIZE);
				io_uring_sqe_set_data(sqe, (void * )(intptr_t)(buffer_index));
				processing_writes++;
				buffer_state[buffer_index] = 2;

				if (cqes[i]->res < BUFFER_SIZE)
				{
					end_reads = true;
				}
			} else
			{
				processing_writes--;
				buffer_state[buffer_index] = 0;
			}
		}
		io_uring_cq_advance(&ring, ended_cqes);

		for (int i = 0; i < URING_ENTRIES; i++)
		{
			if (buffer_state[i] == 0 && !end_reads)
			{
				sqe = io_uring_get_sqe(&ring);
				if (sqe == NULL)
				{
					report_error("io_uring_get_sqe", errno);
					return -1;
				}

				io_uring_prep_read(sqe, in,
					buffers[i], BUFFER_SIZE,
					(read_blocks_processed + processing_reads) * BUFFER_SIZE);
				io_uring_sqe_set_data(sqe, (void * )(intptr_t)(i));
				buffer_offset_block_id[i] = (read_blocks_processed + processing_reads);
				buffer_state[i] = 1;
				processing_reads++;
			}
		}
		io_uring_submit(&ring);
	}
	for (int i=0; i < URING_ENTRIES; i++)
	{
		free(buffers[i]);
	}
	free(buffers);
	io_uring_queue_exit(&ring);
	return 0;
}