/*
 * WRITEN TIME 	:  	May 22 2014
 * AUTHOR	:	Xiao Wenjian
 * DESCRIPTION	:	A disk driver with disk cache
 * FUNCTION	:	Complete the physical disk layer
 */

#include "xph-dev.h"

const char *phd_name = PHD_NAME;

extern struct xlg_dev *dev;

/**
 *read_xph_endio - Callback function after ending reading the physical disk;
 */
void read_xph_endio(struct bio *bio, int err) 
{
	struct cache_block *blk = bio->bi_private;
	set_bit(UPTODATE_BIT, &blk->flag);
	wake_up(&blk->waitq);
}

/**
 *write_xph_endio - Callback function after ending writing the physical disk;
 */
void write_xph_endio(struct bio *bio, int err)
{
	struct cache_block *blk = bio->bi_private;

	clear_bit(DIRTY_BIT, &blk->flag);
	recycle_cache_block(blk);
}

/**
 * xph_construct_bio - construct bio according to the cache block;
 */
static struct bio *xph_construct_bio(int dir, struct cache_block *cb)
{
	struct bio *bio;
	
	bio = bio_alloc(GFP_KERNEL, 1);
	if (!bio)
		return bio;
	bio->bi_sector = cb->sector;
	bio->bi_bdev = dev->bdev;
	bio->bi_rw = dir;
	bio->bi_vcnt = 1;
	bio->bi_idx = 0;
	bio->bi_size = 4096;
	bio->bi_io_vec[0].bv_offset = 0;
	bio->bi_io_vec[0].bv_len = 4096;
	bio->bi_io_vec[0].bv_page = cb->page;
	bio->bi_private = cb;
	if (dir == WRITE) {
		bio->bi_end_io = write_xph_endio;
	}
	else if (dir == READ) {
		bio->bi_end_io = read_xph_endio;
	}

	return bio;
}

/**
 *xph_bio_xfer - Data copy between physical disk and cache block;
 */
int  xph_bio_xfer(int dir, struct cache_block *cb)
{
	struct bio *bio;
	int ret = 0;

	bio = xph_construct_bio(dir, cb);
	if (!bio) {
		ret = -ENOMEM;
		if (dir == WRITE)
			clear_bit(WRITING_BACK_BIT, &cb->flag);
		else {
			set_bit(UPTODATE_BIT, &cb->flag);
			wake_up(&cb->waitq);
		}
		return ret;
	}
	/* block/block-core.c */
	submit_bio(dir, bio);
	return ret;
}
/**
 *xph_init - open the block device;
 */
struct block_device *xph_init(void)
{
	/* fs/block.c  linux/fs.h*/
	dev->bdev = blkdev_get_by_path(phd_name, 0, NULL);	
	return dev->bdev;
}

/**
 *xph_destroy - close the block device
 */
void xph_destroy(void)
{
	blkdev_put(dev->bdev, 0);
	dev->bdev = NULL;
}
