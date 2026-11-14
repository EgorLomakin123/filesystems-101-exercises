#include <solution.h>
#include <fs_malloc.h>
#include <stdint.h>
#include <linux/types.h>
#include <linux/byteorder/little_endian.h>

#include <errno.h>

#define BOOT_BLOCK_SIZE 1024
#define SUPERBLOCK_SIZE 1024

struct ext2_superblock
{
	__le32 inodes_count;
	__le32 blocks_count;
	__le32 not_needed_data1[4];
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
	__le32 block[15];
};

// I will store all blockgroup_descriptors in ram, 
// as i calculated it will be near half of kb, so it is not too much
struct ext2_fs
{
	int fd;
	struct ext2_superblock sb;
	struct ext2_blockgroup_descriptor *bgd;
};

struct ext2_blkiter
{
	/* implement me */
};


int ext2_read_superblock(struct ext2_superblock *sb, int fd)
{
	int r = pread(fd, sb, sizeof(struct ext2_superblock), BOOT_BLOCK_SIZE);
	if (r < 0)
	{
		return -errno;
	}
	else if (r != sizeof(struct ext2_superblock))
	{
		return -errno;
	}
	else if (sb->fs_state != 1 && sb->fs_error_handling == 3)
	{
		return -EPROTO;
	}
	return 0;
}

int ext2_read_blockgroup_descriptor(struct ext2_superblock *sb, int fd, int group, struct ext2_blockgroup_descriptor *bgd)
{
	/* implement me */
	(void) sb;
	(void) fd;
	(void) group;
	(void) bgd;
	return -EOPNOTSUPP;
}

int ext2_fs_init(struct ext2_fs **fs, int fd)
{
	struct ext2_superblock* sb = fs_malloc(sizeof(struct ext2_fs));

	(void) fs;
	(void) fd;
	return -EOPNOTSUPP;
}

void ext2_fs_free(struct ext2_fs *fs)
{
	/* implement me */
	(void) fs;
}

int ext2_blkiter_init(struct ext2_blkiter **i, struct ext2_fs *fs, int ino)
{
	/* implement me */
	(void) i;
	(void) fs;
	(void) ino;
	return -EOPNOTSUPP;
}

int ext2_blkiter_next(struct ext2_blkiter *i, int *blkno)
{
	/* implement me */
	(void) i;
	(void) blkno;
	return 0;
}

void ext2_blkiter_free(struct ext2_blkiter *i)
{
	/* implement me */
	(void) i;
}
