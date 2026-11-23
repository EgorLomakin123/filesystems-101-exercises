#include <ext2_blkiter.h>

#include <stdint.h>
#include <linux/types.h>
#include <linux/byteorder/little_endian.h>
#include <stdbool.h>
#include <fs_malloc.h>
#include <unistd.h>

#include <errno.h>

#define BOOT_BLOCK_SIZE 1024
#define SUPERBLOCK_SIZE 1024
#define LOG_SCALE 1024U
#define BITS_PER_BYTE 8
#define DIRECT_BLOCK_NUMBER 12
#define SINGLE_INDIRECT_BLOCK_NUMBER 1
#define DOUBLE_INDIRECT_BLOCK_NUMBER 1
#define TRIPLE_INDIRECT_BLOCK_NUMBER 1
#define BLOCK_GROUP_DESCRIPTOR_SIZE 32

struct ext2_superblock
{
	__le32 inodes_count;
	__le32 blocks_count;
	__le32 not_needed_data1[3];
	__le32 superblock_block;
	__le32 log_block_size;
	__le32 log_fragment_size;
	__le32 blocks_per_group;
	__le32 fragments_per_group;
	__le32 inodes_per_group;
	__le32 not_needed_data2[2];
	__le16 not_needed_data3[3];
	__le16 fs_state;
	__le16 fs_error_handling;
	__le16 not_needed_data4;
	__le32 not_needed_data5[4];
	__le16 not_needed_data6[2];
	__le32 not_needed_data7;
	__le16 inode_size;
};

struct ext2_blockgroup_descriptor
{
	__le32 block_bitmap_address;
	__le32 inode_bitmap_address;
	__le32 inode_table_address;
};

struct ext2_inode
{
	__le16 mode;
	__le16 uid;
	__le32 size;
	__le32 not_needed_data1[4];
	__le16 not_needed_data2[2];
	__le32 not_needed_data3[3];
	__le32 block_pointers[15];
};

enum ext2_inode_layer
{
	IN_INODE_READING = 0,
	LAYER_1 = 1,
	LAYER_2 = 2,
	LAYER_3 = 3
};

// I will store all blockgroup_descriptors in ram, 
// as i calculated it will be near half of kb, so it is not too much
struct ext2_fs
{
	int fd;
	struct ext2_superblock sb;
	struct ext2_blockgroup_descriptor *bgds;
	unsigned long block_size;
};

struct ext2_blkiter
{
	struct ext2_fs *fs;
	struct ext2_inode *inode;
	int blocks_to_read;
	int max_pointers_in_block;
	int cur_inode_index;
	enum ext2_inode_layer cur_layer;
	__le32* cur_block[3];
	int cur_block_index[3];
	bool is_layer_read[3];
};

// group - 0 based
unsigned long calculate_blockgroup_offset(struct ext2_fs* fs, __le32 group)
{
	unsigned long blockgroup_block = fs->sb.superblock_block + 1;
	unsigned long blockgroup_offset = blockgroup_block * fs->block_size + (unsigned long)group * BLOCK_GROUP_DESCRIPTOR_SIZE;
	return blockgroup_offset;
}

unsigned long calculate_block_offset(struct ext2_fs *fs, __le32 block_group, __le32 block_index)
{
	unsigned long block_offset = (unsigned long)block_group * fs->block_size * fs->sb.blocks_per_group + (unsigned long)block_index * fs->block_size;
	return block_offset;
}

unsigned long ext2_fs_get_block_size(struct ext2_fs *fs)
{
    return fs->block_size;
}

unsigned int ext2_fs_get_blocks_per_group(struct ext2_fs *fs)
{
    return fs->sb.blocks_per_group;
}

unsigned long ext2_blkiter_get_file_size(struct ext2_blkiter *i)
{
    return i->inode->size;
}

unsigned long calculate_block_offset_by_block_number(struct ext2_fs *fs, unsigned int blkno)
{
    __le32 block_size = ext2_fs_get_blocks_per_group(fs);
    __le32 block_group = blkno / block_size;
    __le32 block_index = blkno % block_size;
    return calculate_block_offset(fs, block_group, block_index);
}



int ext2_read_superblock(struct ext2_superblock *sb, int fd)
{
	int r = pread(fd, sb, sizeof(struct ext2_superblock), BOOT_BLOCK_SIZE);
	if (r < 0)
	{
		return -errno;
	}
	else if (r != sizeof(struct ext2_superblock))
	{
		return -EPROTO;
	}
	else if (sb->fs_state != 1 && sb->fs_error_handling == 3)
	{
		return -EPROTO;
	}
	return 0;
}

// group - 0 based
int ext2_read_blockgroup_descriptor(struct ext2_fs *fs, __le32 group, struct ext2_blockgroup_descriptor *bgd)
{
	unsigned long bgd_offset = calculate_blockgroup_offset(fs, group);
	int r = pread(fs->fd, bgd, sizeof(struct ext2_blockgroup_descriptor), bgd_offset);
	if (r < 0)
	{
		return -errno;
	}
	else if (r != sizeof(struct ext2_blockgroup_descriptor))
	{
		return -EPROTO;
	}
	return 0;
}

int ext2_check_inode_in_use(struct ext2_fs *fs, __le32 group, __le32 inode_index_in_group)
{
	uint8_t bitmap[fs->block_size];
	__le32 inode_bitmap_block = fs->bgds[group].inode_bitmap_address;
	__le32 inode_bitmap_block_group = inode_bitmap_block / fs->sb.blocks_per_group;
	__le32 inode_bitmap_block_index = inode_bitmap_block % fs->sb.blocks_per_group;
	unsigned long inode_bitmap_offset = calculate_block_offset(fs, inode_bitmap_block_group, inode_bitmap_block_index);
	int r = pread(fs->fd, bitmap, fs->block_size, inode_bitmap_offset);
	if (r < 0)
	{
		return -errno;
	}
	bool is_in_use = (bitmap[inode_index_in_group / 8] >> (inode_index_in_group % 8)) & 1;
	if (!is_in_use)
	{
		return -ENOENT;
	}
	return 0;
}

// ino - 1-based
int ext2_read_inode(struct ext2_fs *fs, int ino, struct ext2_inode *inode)
{
	if (ino < 1)
	{
		return -EINVAL;
	}
	if ((__le32)ino > fs->sb.inodes_count)
	{
		return -EINVAL;
	}
	__le32 actual_ino_index = ino - 1;
	__le32 group = actual_ino_index / fs->sb.inodes_per_group;
	__le32 inode_index_in_group = actual_ino_index % fs->sb.inodes_per_group;
	int r = ext2_check_inode_in_use(fs, group, inode_index_in_group);
	if (r < 0)
	{
		return r;
	}
	unsigned long inode_table_offset = fs->bgds[group].inode_table_address * fs->block_size;
	__le16 inode_size = fs->sb.inode_size;
	unsigned long inode_offset = inode_table_offset + inode_index_in_group * inode_size;
	r = pread(fs->fd, inode, sizeof(struct ext2_inode), inode_offset);
	if (r < 0)
	{
		return -errno;
	}
	return 0;
}

int ext2_read_block(struct ext2_fs *fs, __le32 block, void *buf)
{
	__le32 block_group = block / fs->sb.blocks_per_group;
	__le32 block_index = block % fs->sb.blocks_per_group;
	if (block >= fs->sb.blocks_count)
	{
		return -EPROTO;
	}
	unsigned long block_offset = calculate_block_offset(fs, block_group, block_index);
	int r = pread(fs->fd, buf, fs->block_size, block_offset);
	if (r < 0)
	{
		return -errno;
	}
	else if ((__le32)r != fs->block_size)
	{
		return -EPROTO;
	}
	return 0;
}

int ext2_fs_init(struct ext2_fs **fs, int fd)
{
	struct ext2_fs* new_fs = fs_xmalloc(sizeof(struct ext2_fs));
	*fs = new_fs;
	new_fs->fd = fd;
	new_fs->bgds = NULL;
	struct ext2_superblock *sb = &new_fs->sb;
	int r = ext2_read_superblock(sb, fd);
	if (r < 0)
	{
		ext2_fs_free(new_fs);
		return r;
	}
	new_fs->block_size = LOG_SCALE << sb->log_block_size;
	int amount_of_blockgroups = sb->blocks_count / sb->blocks_per_group;
	if (sb->blocks_count % sb->blocks_per_group != 0)
	{
		amount_of_blockgroups++;
	}
	struct ext2_blockgroup_descriptor *bgds = fs_xmalloc(sizeof(struct ext2_blockgroup_descriptor) * amount_of_blockgroups);
	new_fs->bgds = bgds;
	for(int i = 0; i < amount_of_blockgroups; i++)
	{
		r = ext2_read_blockgroup_descriptor(new_fs, i, &bgds[i]);
		if (r < 0)
		{
			ext2_fs_free(new_fs);
			return r;
		}
	}
	return 0;
}

void ext2_fs_free(struct ext2_fs *fs)
{
	if (fs == NULL)
	{
		return;
	}
	if (fs->bgds != NULL)
	{
		fs_xfree(fs->bgds);
	}
	close(fs->fd);
	fs_xfree(fs);
}

// files more then 4 gb are not supported (size in bytes must be less then 2^32)
int ext2_blkiter_init(struct ext2_blkiter **i, struct ext2_fs *fs, int ino)
{
	struct ext2_blkiter* new_iter = fs_xmalloc(sizeof(struct ext2_blkiter));
	*i = new_iter;
	new_iter->fs = fs;
	new_iter->inode = fs_xmalloc(sizeof(struct ext2_inode));
	int r = ext2_read_inode(fs, ino, new_iter->inode);
	if (r < 0)
	{
		ext2_blkiter_free(new_iter);
		return r;
	}
	new_iter->max_pointers_in_block = fs->block_size / sizeof(__le32);
	new_iter->blocks_to_read = new_iter->inode->size / fs->block_size;
	if (new_iter->inode->size % fs->block_size != 0)
	{
		new_iter->blocks_to_read++;
	}
	new_iter->cur_inode_index = 0;

	new_iter->cur_layer = IN_INODE_READING;
	for (int i = 0; i < 3; i++)
	{
		new_iter->cur_block[i] = fs_xmalloc(new_iter->max_pointers_in_block * sizeof(__le32));
		new_iter->cur_block_index[i] = 0;
		new_iter->is_layer_read[i] = false;
	}
	return 0;
}

int ext2_blkiter_next(struct ext2_blkiter *iter, int *blkno)
{
	if (iter->blocks_to_read == 0)
	{
		return 0;
	}

	if (iter->cur_layer == IN_INODE_READING)
	{
		if (iter->inode->block_pointers[iter->cur_inode_index] >= iter->fs->sb.blocks_count)
		{
			return -EPROTO;
		}
		*blkno = iter->inode->block_pointers[iter->cur_inode_index];
		iter->blocks_to_read--;
		iter->cur_inode_index++;
		if (iter->cur_inode_index == DIRECT_BLOCK_NUMBER)
		{
			iter->is_layer_read[LAYER_1-1] = true;
			iter->cur_layer++;
		}
		return 1;
	}
	int r;

	if (iter->is_layer_read[0])
	{
		iter->is_layer_read[0] = false;
		iter->cur_block_index[0] = 0;
		fs_xfree(iter->cur_block[0]);
		iter->cur_block[0] = fs_xmalloc(iter->max_pointers_in_block * sizeof(__le32));
		r = ext2_read_block(iter->fs, iter->inode->block_pointers[iter->cur_inode_index], iter->cur_block[0]);
		if (r < 0)
		{
			return r;
		}
	}
	for (int i = 1; i < LAYER_3; i++)
	{
		if (iter->is_layer_read[i])
		{
			iter->is_layer_read[i] = false;
			iter->cur_block_index[i] = 0;
			fs_xfree(iter->cur_block[i]);
			iter->cur_block[i] = fs_xmalloc(iter->max_pointers_in_block * sizeof(__le32));
			r = ext2_read_block(iter->fs, iter->cur_block[i - 1][iter->cur_block_index[i - 1]], iter->cur_block[i]);
			if (r < 0)
			{
				return r;
			}
		}
	}

	if (iter->cur_block[iter->cur_layer-1][iter->cur_block_index[iter->cur_layer-1]] >= iter->fs->sb.blocks_count)
	{
		return -EPROTO;
	}
	*blkno = iter->cur_block[iter->cur_layer-1][iter->cur_block_index[iter->cur_layer-1]];
	iter->blocks_to_read--;
	r = 1;

	iter->cur_block_index[iter->cur_layer-1]++;
	iter->is_layer_read[iter->cur_layer-1] = iter->cur_block_index[iter->cur_layer-1] == iter->max_pointers_in_block;
	for (int j=iter->cur_layer-1; j > 0; j--)
	{
		if (iter->is_layer_read[j])
		{
			iter->cur_block_index[j-1]++;
			iter->is_layer_read[j-1] = iter->cur_block_index[j-1] == iter->max_pointers_in_block;
		}
	}
	if (iter->is_layer_read[LAYER_1-1])
	{
		iter->cur_inode_index++;
		iter->cur_layer++;
		for (int i = 1; i < (int)iter->cur_layer; i++)
		{
			iter->is_layer_read[i] = true;
		}
	} 
	return r;
}

void ext2_blkiter_free(struct ext2_blkiter *i)
{
	if (i == NULL)
	{
		return;
	}
	if (i->inode != NULL)
	{
		fs_xfree(i->inode);
	}
	for (int j=0; j < LAYER_3; j++){
		if (i->cur_block[j] != NULL)
		{
			fs_xfree(i->cur_block[j]);
		}
	}
	fs_xfree(i);
}
