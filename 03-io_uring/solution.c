#include <solution.h>
#include <liburing.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define URING_ENTRIES 4
#define BUFFER_SIZE (1024 * 256)
enum buffer_status
{
	BUFFER_STATE_FREE = 0,
	BUFFER_STATE_READING = 1,
	BUFFER_STATE_WRITING = 2,
};

struct buffer_state
{
	enum buffer_status status;
	bool end_of_read;
	long last_read_offset_ind;
};


int copy(int in, int out)
{
	(void) in;
	(void) out;

	int return_code = 0;
	int processing_reads = 0;
	int processing_writes = 0;
	long read_blocks_processed = 0;

	char **buffers = malloc(URING_ENTRIES * sizeof(*buffers));
	for (size_t i = 0; i < URING_ENTRIES; i++)
	{
		buffers[i] = malloc(BUFFER_SIZE);
	}
	
	struct buffer_state buffer_states[URING_ENTRIES];
	for (int i = 0; i < URING_ENTRIES; i++)
	{
		buffer_states[i].status = BUFFER_STATE_FREE;
		buffer_states[i].end_of_read = false;
		buffer_states[i].last_read_offset_ind = -1;
	}

	struct io_uring ring;
	int uring_init_res = io_uring_queue_init(URING_ENTRIES, &ring, 0);
	if (uring_init_res != 0 )
	{
		return_code = uring_init_res;
		goto out_only_free_buffers;
	}
	struct io_uring_sqe* sqe;
	struct io_uring_cqe* cqe;
	int prepared_operations = 0;
	for (; processing_reads < URING_ENTRIES; processing_reads++)
	{
		sqe = io_uring_get_sqe(&ring);
		io_uring_prep_read(sqe, in,
			buffers[processing_reads], BUFFER_SIZE,
			(read_blocks_processed + processing_reads) * BUFFER_SIZE);
		io_uring_sqe_set_data(sqe, (void * )(intptr_t)(processing_reads));
		prepared_operations++;
		buffer_states[processing_reads].last_read_offset_ind = (read_blocks_processed + processing_reads);
		buffer_states[processing_reads].status = BUFFER_STATE_READING;
	}
	int uring_submit_res = io_uring_submit(&ring);
	if (uring_submit_res != prepared_operations)
	{
		return_code = uring_submit_res;
		goto out;
	}
	prepared_operations = 0;

	while (processing_reads + processing_writes > 0)
	{
		int uring_wait_res = io_uring_wait_cqe(&ring, &cqe);
		if (uring_wait_res != 0)
		{
			return_code = uring_wait_res;
			goto out;
		}

		struct io_uring_cqe* cqes[URING_ENTRIES];
		unsigned int ended_cqes = io_uring_peek_batch_cqe(&ring, cqes, URING_ENTRIES);
		for (unsigned int i = 0; i < ended_cqes; i++)
		{
			int buffer_index = (int)(intptr_t)io_uring_cqe_get_data(cqes[i]);
			if (cqes[i]->res < 0)
			{
				return_code = cqes[i]->res;
				goto out;
			}
			if (buffer_states[buffer_index].status == BUFFER_STATE_READING)
			{
				processing_reads --;
				read_blocks_processed++;

				struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
				io_uring_prep_write(sqe, out,
				 	buffers[buffer_index], cqes[i]->res,
				  	(buffer_states[buffer_index].last_read_offset_ind) * BUFFER_SIZE);
				io_uring_sqe_set_data(sqe, (void * )(intptr_t)(buffer_index));

				prepared_operations++;
				processing_writes++;
				buffer_states[buffer_index].status = BUFFER_STATE_WRITING;

				if (cqes[i]->res < BUFFER_SIZE)
				{
					buffer_states[buffer_index].end_of_read = true;
				}
			} else if (buffer_states[buffer_index].status == BUFFER_STATE_WRITING )
			{
				processing_writes--;
				if (buffer_states[buffer_index].end_of_read) {
					buffer_states[buffer_index].status = BUFFER_STATE_FREE;
					continue;
				}

				sqe = io_uring_get_sqe(&ring);
				io_uring_prep_read(sqe, in,
					buffers[buffer_index], BUFFER_SIZE,
					(read_blocks_processed + processing_reads) * BUFFER_SIZE);
				io_uring_sqe_set_data(sqe, (void * )(intptr_t)(buffer_index));

				prepared_operations++;
				buffer_states[buffer_index].last_read_offset_ind = (read_blocks_processed + processing_reads);
				buffer_states[buffer_index].status = BUFFER_STATE_READING;
				processing_reads++;
			}
		}
		io_uring_cq_advance(&ring, ended_cqes);
		int uring_submit_res = io_uring_submit(&ring);
		if (uring_submit_res != prepared_operations)
		{
			return_code = uring_submit_res;
			goto out;
		}
		prepared_operations = 0;
	}

out:
	io_uring_queue_exit(&ring);
out_only_free_buffers:
	for (int i=0; i < URING_ENTRIES; i++)
	{
		free(buffers[i]);
	}
	free(buffers);
	return return_code;
}