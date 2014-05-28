#ifndef XCACHE_DEV
#define XCACHE_DEV

#include "dev-args.h"
#include <linux/radix-tree.h>				/* radix_tree operations */
#include <linux/wait.h>
#include <linux/list.h>					/* list_head */

#define DIRTY_BIT 		0			/* dirty bit of the flag; */
#define WRITING_BACK_BIT 	1			/* writing back bit of the flag; */
#define LOCK_BIT		2			/* lock bit of the flag; */
#define UPTODATE_BIT		3			/* up to date bit of the flag; */

struct cache_block {
	struct list_head 	block_list;		/* block list; */
	unsigned long 		sector;			/* The starting sector number of the block; */
	struct page		*page;			/* the page volumn of the block for storing; */
	atomic_t		ref_cnt;		/* the reference count of the blcock; */
	wait_queue_head_t	waitq;			/* waitqueue for thread waiting for someting; */
	unsigned long		flag;			/* the status flag of the block; */
	struct work_struct	update_work;		/* the work of updating the cache block */
};

struct cache_zone {
	struct list_head 	cache_blocks;		/* list linking the cached blocks; */
	struct radix_tree_root 	rdtree;			/* radix tree for managing the cached blocks; */
	spinlock_t 		lock;			/* spinlock for protecting list_head; */
};

struct free_zone {
	struct list_head	free_blocks;		/* list linking the free blocks; */
	atomic_t		size;			/* the num of free blocks; */
	spinlock_t		lock;			/* spinlock for protecting list_head; */
};

struct xcache {
	struct cache_zone 	cache_zone;		/* the zone containing the cached blocks; */
	struct free_zone  	free_zone;		/* the zone linking the free blocks; */
	struct work_struct 	flush_work;		/* the work of flushing the disk */
	wait_queue_head_t  	waitq;			/* waitqueue for thread waiting; */
};

void recycle_cache_block(struct cache_block *blk);

int xcache_xfer(struct bio_vec *bv, sector_t sector, unsigned long dir);

int xcache_init(void);

int xcache_destroy(void);

#endif
