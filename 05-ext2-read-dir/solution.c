#include <solution.h>
#include <linux/types.h>
#include <ext2_blkiter.h>
#include <errno.h>
#include <string.h>
#include <fs_malloc.h>
#include <unistd.h>

#define DIR_ENTRY_SIZE_NO_NAME sizeof(__le32) + sizeof(__le16) + sizeof(__u8) * 2
#define DIR_ENTRY_TYPE_FILE 'f'
#define DIR_ENTRY_TYPE_DIR 'd'
#define DIR_ENTRY_MAXIMAL_NAME_LENGTH 2^8

struct directory_entry
{
	__le32 inode;
	__le16 size;
	__u8 name_length;
	__u8 file_type;
	char name[];

};

void parse_directory_entry(void *buffer, int entry_offset, struct directory_entry* entry)
{
	memcpy(&entry->inode, buffer + entry_offset, DIR_ENTRY_SIZE_NO_NAME);
	memcpy(&entry->name, buffer + entry_offset + DIR_ENTRY_SIZE_NO_NAME, entry->name_length);

}

int dump_dir(int img, int inode_nr)
{
	int r;
	struct ext2_fs *fs = NULL;
	struct ext2_blkiter *i = NULL;

	if ((r = ext2_fs_init(&fs, dup(img))))
	{
		return r;
	}
		
	if ((r = ext2_blkiter_init(&i, fs, inode_nr)))
	{
		ext2_fs_free(fs);
		return r;
	}

	unsigned int block_size = ext2_fs_get_block_size(fs);
	unsigned long size_to_read = ext2_blkiter_get_file_size(i);
	void *buff = fs_xmalloc(block_size);
	int blocks_read = 0;

	int blkno;
	struct directory_entry *dir_entry = malloc(DIR_ENTRY_SIZE_NO_NAME+DIR_ENTRY_MAXIMAL_NAME_LENGTH);
	while ((r = ext2_blkiter_next(i, &blkno)) != 0) {
		if (r < 0)
		{
			goto out;
		}

		unsigned long blk_offset = calculate_block_offset_by_block_number(fs, blkno);
		size_t read_size = block_size;
		if (size_to_read < (unsigned long)block_size * (blocks_read + 1))
		{
			read_size = size_to_read - (unsigned long)block_size * blocks_read;
		}

		r = pread(img, buff, read_size, blk_offset);
		if (r < 0)
		{
			r = -errno;
			goto out;
		}
		blocks_read++;

		unsigned int entry_offset = 0;
		while (entry_offset < block_size)
		{
			parse_directory_entry(buff, entry_offset, dir_entry);
			entry_offset += dir_entry->size;
			const char* name = dir_entry->name;
			if (dir_entry->file_type == 1 && dir_entry->inode != 0)
			{
				report_file(dir_entry->inode, DIR_ENTRY_TYPE_FILE, name);
			} else if (dir_entry->file_type == 2 && dir_entry->inode != 0)
			{
				report_file(dir_entry->inode, DIR_ENTRY_TYPE_DIR, name);
			}
		}
	}

out:
	fs_xfree(buff);
	ext2_blkiter_free(i);
	ext2_fs_free(fs);
	return r;
}
