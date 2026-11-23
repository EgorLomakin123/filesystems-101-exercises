#include <solution.h>
#include <ext2_blkiter.h>
#include <fs_malloc.h>
#include <errno.h>

int dump_file(int img, int inode_nr, int out)
{
	(void) img;
	(void) inode_nr;
	(void) out;

	/* implement me */
	int r;
	struct ext2_fs *fs = NULL;
	struct ext2_blkiter *i = NULL;
	unsigned long write_offset = 0;

	if (r = ext2_fs_init(&fs, dup(img)))
	{
		return -errno;
	}
		
	if (r = ext2_blkiter_init(&i, fs, inode_nr))
	{
		ext2_fs_free(fs);
		return -errno;
	}

	unsigned long block_size = ext2_fs_get_block_size(fs);
	unsigned long size_to_read = ext2_blkiter_get_file_size(i);
	unsigned int blocks_per_group = ext2_fs_get_blocks_per_group(fs);
	int *buff = fs_xmalloc(block_size);

	for (;;) {
		int blkno;
		r = ext2_blkiter_next(i, &blkno);
		if (r < 0)
		{
			r = -errno;
			goto out;
		}
		if (r == 0)
		{
			r = -errno;
			goto out;
		}
		unsigned int blk_number = blkno / blocks_per_group;
		unsigned int blk_index = blkno % blocks_per_group;
		unsigned long blk_offset = calculate_block_offset(fs, blk_number, blk_index);

		size_t read_size = block_size;
		if (size_to_read < block_size)
		{
			read_size = size_to_read;
		}

		r = pread(img, buff, read_size, blk_offset);
		if (r < 0)
		{
			r = -errno;
			goto out;
		}

		size_to_read -= block_size;
		size_t write_size = r;
		
		r = pwrite(img, buff, write_size, write_offset);
		if (r < 0)
		{
			r = -errno;
			goto out;
		}

		write_offset += block_size;
	}

out:
	fs_xfree(buff);
	ext2_blkiter_free(i);
	ext2_fs_free(fs);
	return r;
}
