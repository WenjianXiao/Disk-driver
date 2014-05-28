/*
 * WRITEN TIME 	:  	May 22 2014
 * AUTHOR	:	Xiao Wenjian
 * DESCRIPTION	:	A disk driver with disk cache
 * FUNCTION	:	Complete the cache layer
 */

#include "xlg-dev.h"
#include "xcache-dev.h"
#include <linux/gfp.h>					/* alloc page */
#include <linux/workqueue.h>				/* work_struct */

extern struct xlg_dev *dev;				/* The logical device */

/**
 *get_cache_block - Get a cache block from the free zone;
 *
 * @fz:	Pointer to the free_zone;
 *
 * Return: Pointer to the cache block; NULL if having no free cache block;
 */
struct cache_block *get_cache_block(struct free_zone *fz)
{
	struct cache_block *cb = NULL;
	unsigned long flag = 0;
	struct list_head *entry = fz->free_blocks.next;
	if (!entry)
		return NULL;
	spin_lock_irqsave(&fz->lock, flag);
	list_del_init(entry);
	atomic_dec(&fz->size);
	spin_unlock_irqrestore(&fz->lock, flag);
	cb = list_entry(entry, struct cache_block, block_list);
	return cb;
}

/**
 *insert_block_to_radix_tree - Insert block to the radix tree
 *
 * @cb: Pointer to the cache block;
 * @rdtree: Pointer to the radix tree root;
 * &sector: As the index of the cache block;
 *
 * Return: 0 if successfully,Or error code if failed;
 */
static inline int insert_block_to_radix_tree(struct cache_block *cb, struct radix_tree_root *rdtree, 
		sector_t sector)
{
	int ret = 0;
	radix_tree_preload(GFP_ATOMIC);
	ret = radix_tree_insert(rdtree, sector, cb);
	radix_tree_preload_end();
	return ret;
}

/**
 *free_blocks_add - add the cache block to the free zone
 */
static void free_blocks_add(struct cache_block *blk, struct free_zone *fz) 
{
	unsigned long flag;

	spin_lock_irqsave(&fz->lock, flag);
	list_add_tail(&blk->block_list, &fz->free_blocks);
	atomic_inc(&fz->size);
	wake_up(&dev->cache.waitq);
	spin_unlock_irqrestore(&fz->lock, flag);
}

/**
 *cache_blocks_add - Add the cache block to the cache zone;
 *
 * @blk: The cache block;
 * @cz: The cache zone;
 */
static void cache_blocks_add(struct cache_block *blk, struct cache_zone *cz)
{
	/*
	 * Having no spinlock because of having spin_locked before calling it
	 * in function find_cache_block. The spin lock can't be recursive;
	 */
	list_move(&blk->block_list, &cz->cache_blocks);
}
/**
 *find_cache_block - find a cache block by sector as the index.
 *Firstly find it in the radix tree.If not found,get a new cache
 *block from free zone and add it to the radix tree and the cache zone;
 *
 * @sector: As the index of the block;
 * @cb: Pointer to Pointer to cache block.After the function end,if *cb == NULL
 * means failed;
 *
 * Return values:
 *0: The cache block has been in the radix tree.
 *1: The cache block is from free zone.
 */
static int find_cache_block(sector_t sector, struct cache_block **cb)
{
	int ret = 0;
	unsigned long flag = 0;
	int err = 0;
	struct xcache *cache = &dev->cache;
	struct cache_zone *cz = &cache->cache_zone;
	struct free_zone *fz = &cache->free_zone;

	printk(KERN_ALERT "------>find_cache_block()");
	/* 
	 * Everytime check whether the cache block is in the radix_tree firstly. 
	 * Avoid the conflict of the block in the radix tree.
	 */
retry:
	wait_event(cache->waitq, (atomic_read(&fz->size)) > 0);
	printk(KERN_ALERT "------>wait_free_zone");

	spin_lock_irqsave(&cz->lock, flag);
	*cb = radix_tree_lookup(&cz->rdtree, sector);
	if (!*cb) {
		*cb = get_cache_block(fz);
		if (!*cb) {
			spin_unlock_irqrestore(&cz->lock, flag);
			goto retry;
		}
		(*cb)->sector = sector;
		err = insert_block_to_radix_tree(*cb, &cz->rdtree, sector);
		if (err) {
			free_blocks_add(*cb, fz);
			*cb = NULL;
			goto unlock;
		}
		ret = 1;
	}
	atomic_inc(&(*cb)->ref_cnt);
	cache_blocks_add(*cb, cz);
unlock:
	spin_unlock_irqrestore(&cz->lock, flag);
	printk(KERN_ALERT "<------find_cache_block");
	return ret;
}

/**
 *cache_block_xfer - transter the data between the page of bv and the cache block
 *
 * @bv: the bio_vec of the bio;
 * @cb: Pointer to the cache block;
 * @dir: READ or WRITE;
 */
static void cache_block_xfer(struct bio_vec *bv, struct cache_block
		*cb, int dir)
{
	char *buffer = page_address(bv->bv_page);
	char *cache = page_address(cb->page);
	printk(KERN_ALERT "------>cache_block_xfer:");
	if (dir == READ)
		printk(KERN_ALERT "READ sectore :%lu",cb->sector);
	else 
		printk(KERN_ALERT "WRITE sectore :%lu", cb->sector);
	while (!test_bit(UPTODATE_BIT, &cb->flag)) 
		wait_event(cb->waitq, test_bit(UPTODATE_BIT, &cb->flag));
	if (dir == READ) {
		printk(KERN_ALERT "------>cache_block_read");
		memcpy(buffer+bv->bv_offset, cache+bv->bv_offset, 
			bv->bv_len);
	}
	else {
		printk(KERN_ALERT "------>cache_block_write");
		memcpy(cache+bv->bv_offset, buffer+bv->bv_offset,
				bv->bv_len);
		set_bit(DIRTY_BIT, &cb->flag);
	}
}

/**
 *
 */
static int wait_schedule(void *data)
{
	if(signal_pending(current))
		return -ERESTARTSYS;
	schedule();
	return 0;
}

/**
 *xcache_xfer - Complete the data copy between the logical device and the cache device
 *
 *@bv: the bio_vec contained in the bio
 *@sector: the start sector of the cache block;
 *@dir: READ or Write;
 *
 */
int xcache_xfer(struct bio_vec *bv, sector_t sector, unsigned long dir)
{
	struct cache_block *cb = NULL;
	int ret = 0;

	printk(KERN_ALERT "------>xcache_xfer()");
	ret = find_cache_block(sector, &cb);
	if (!cb) 
		goto failed;

	printk(KERN_ALERT "------>wait lock");
	/* check whether the block has been locked. If not,lock it.Or wait */
	while (test_and_set_bit(LOCK_BIT, &cb->flag) ) {
		wait_event(cb->waitq, !test_bit(LOCK_BIT, &cb->flag));
	}
	
	printk(KERN_ALERT "------>end wait lock");
	
	/* 
	 *ret == 1 imply the data is not in the cache zone. So firstly update 
	 *the block from the lower device. 
	 */

	if (ret) {
		printk(KERN_ALERT "------>from_free_zone");
		if (dir == READ || bv->bv_len < CBLOCK_SIZE) {
			clear_bit(UPTODATE_BIT, &cb->flag);
			printk(KERN_ALERT "------>free_zone_uptodate");
			//xph_bio_read_block(cb);
			schedule_work(&cb->update_work);
		}
		else {
			printk(KERN_ALERT "full block");
			set_bit(UPTODATE_BIT, &cb->flag);
		}

	}
	/*
	 *Wait if the cache block is writing back.
	 */
	else {
		printk(KERN_ALERT "------>wait writing back");
		set_bit(UPTODATE_BIT, &cb->flag);
		wait_on_bit(&cb->flag, WRITING_BACK_BIT, wait_schedule, TASK_INTERRUPTIBLE);
	}
	cache_block_xfer(bv, cb, dir);
	atomic_dec(&cb->ref_cnt);
	clear_bit(LOCK_BIT, &cb->flag);
	wake_up(&cb->waitq);
	/* flush the cache if having no free blocks*/
	schedule_work(&dev->cache.flush_work);
	return 0;
failed:
	return -ENOMEM;
}

/**
 * cache_need_flush - Judge whether the cache needs flushing
 *
 * Return values:
 * 0:	not need flushing
 * 1:	need flushing
 */
static inline int cache_need_flush(struct xcache *cache)
{
	int size;
	DOOR(cache);
	size = atomic_read(&cache->free_zone.size);
	if (size <= (NBLOCKS >> 4))
		return 1;
	return 0;
}

/**
 *Get the coldest cache block. Search the coldest block from cache_zone's tail ,the block
 * must have ref_cnt == 0 and is not writing_back;
 *
 * @cache: Pointer to the cache;
 *
 * Return values: Pointer to the block, or NULL if not finding the block;
 */
static struct cache_block *get_coldest_cache_block(struct xcache *cache) {
	struct cache_block *cb = NULL;
	struct cache_block *next;
	struct cache_zone *cz;
	unsigned long flag = 0;
	
	DOOR(cache);
	cz = &cache->cache_zone;
	spin_lock_irqsave(&cz->lock, flag);
	list_for_each_entry_safe_reverse(cb, next, &cz->cache_blocks, block_list) {
		if (atomic_read(&cb->ref_cnt) > 0)
			continue;
		if (!test_and_set_bit(WRITING_BACK_BIT, &cb->flag)) 
			break;
	}
	spin_unlock_irqrestore(&cz->lock, flag);
	return cb;
}

/**
 *recyle cache block - delete cache block from radixtree and cache zone
 *then add it to the free zone. Lastly, clear the WRITING_BACK bit and 
 *wake up the thread on the waitqueue.
 */
void recycle_cache_block(struct cache_block *colbk) 
{
	struct cache_zone *cz = &dev->cache.cache_zone;
	unsigned long flag;
	
	printk(KERN_ALERT "------>recycle_cache_block");
	spin_lock_irqsave(&cz->lock, flag);
	if (!atomic_read(&colbk->ref_cnt)) {
		struct cache_block *cb = NULL;
		cb = radix_tree_delete(&cz->rdtree, colbk->sector);
		list_del_init(&colbk->block_list);
		spin_unlock_irqrestore(&cz->lock, flag);
		free_blocks_add(colbk, &dev->cache.free_zone);
	}
	else {
		spin_unlock_irqrestore(&cz->lock, flag);
	}
	clear_bit(WRITING_BACK_BIT, &colbk->flag);
	wake_up_bit(&colbk->flag, WRITING_BACK_BIT);
}

/**
 *flush_coldest_cache_block - flush the coldest cache block.
 *If the cache block is dirty, write back the block to the disk.
 *else recycle the block directly;
 */
static void flush_cache_block(struct cache_block *colbk)
{
	if (!colbk)
		return;
	if (test_bit(DIRTY_BIT, &colbk->flag)) {
		printk(KERN_ALERT "flush is writing backe !");
		xph_bio_write_block(colbk);
	}
	else 
		recycle_cache_block(colbk);
}

/**
 * flush_cold_cache_block - Flush the cold cache block
 */
static void flush_cold_cache_block(struct work_struct *work)
{
	struct xcache *cache = &dev->cache;
	printk(KERN_ALERT "------>flush_cold_cache_block");
	if (cache_need_flush(cache)) {
		int i;
		int free_size;
		printk(KERN_ALERT "------>need_flush_cold_cache_block");
		free_size = atomic_read(&cache->free_zone.size);
		for (i = 0; i < (NBLOCKS>>4) - free_size; i++) {
			struct cache_block *colbk = get_coldest_cache_block(cache);
			flush_cache_block(colbk);
		}
	}
	printk(KERN_ALERT "<------END flush_cold_cache_block");
}

/**
 * new_cache_block - Allocate page for cache block and initialize the block;
 *
 * Return: The pointer to the cache_block if successfully, or NULL if failed;
 */
static struct cache_block *new_cache_block(void)
{
	/* New a block structure, alloc a page for it and initialize other data */
	struct cache_block *cb = NULL;
	cb = kmalloc(sizeof(struct cache_block), GFP_KERNEL);
	if (!cb) 
		goto fail_to_alloc_block_structure;
	cb->page = alloc_page(GFP_KERNEL);
	if (!cb->page)
		goto fail_to_alloc_block_page;
	INIT_LIST_HEAD(&cb->block_list);
	atomic_set(&cb->ref_cnt, 0);
	init_waitqueue_head(&cb->waitq);
	return cb;
fail_to_alloc_block_page:
	kfree(cb);
	cb = NULL;
fail_to_alloc_block_structure:
	return cb;
}

/**
 * free_all_cache_blocks - Free all cache blocks in the free zone if alloc failed;
 * @free_blocks	- List of free blocks
 */
static void free_all_cache_blocks(struct list_head *free_blocks)
{
	struct cache_block *cur;
	struct cache_block *next;

	DOOR(free_blocks);
	list_for_each_entry_safe(cur, next, free_blocks, block_list) {
		list_del(&cur->block_list);
		__free_page(cur->page);
		kfree(cur);
	}
	return;
}

/**
 *update_cache_block - update the cache block from the disk
 */
static void update_cache_block(struct work_struct *work) 
{
	struct cache_block *cb;
	cb = list_entry(work, struct cache_block, update_work);
	xph_bio_read_block(cb);
}

/**
 * xcache_init - Initialize the cache structure and alloc cache blocks;
 *
 * Return: 0 means successful and non-0 means failed;
 */
int xcache_init()
{
	struct xcache *cache = &dev->cache;
	struct cache_zone *cz = &cache->cache_zone;
	struct free_zone *fz = &cache->free_zone;
	int i = 0;
	struct cache_block *cb;
	
	printk(KERN_ALERT "------>xcache_init()");
	/* Initialize the cache_zone structure; */
	INIT_LIST_HEAD(&cz->cache_blocks);
	spin_lock_init(&cz->lock);
	INIT_RADIX_TREE(&cz->rdtree, GFP_ATOMIC);
	
	/* Initialize the free_zone structure; */
	INIT_LIST_HEAD(&fz->free_blocks);
	spin_lock_init(&fz->lock);
	atomic_set(&fz->size, 0);
	
	/* Initialize the work and waitqueue */
	INIT_WORK(&cache->flush_work, flush_cold_cache_block);
	init_waitqueue_head(&cache->waitq);
	
	/* Allocate the cache blocks for the cache */
	for (i = 0; i < NBLOCKS; i++) {
		cb = new_cache_block();
		if (!cb)
			goto new_cache_block_failed;
		atomic_set(&cb->ref_cnt, 0);
		cb->flag = 0;
		INIT_WORK(&cb->update_work, update_cache_block);
		list_add(&cb->block_list, &fz->free_blocks);
		atomic_inc(&fz->size);
	}
	printk(KERN_ALERT "<------end xcache_init()");
	return 0;
new_cache_block_failed:
	free_all_cache_blocks(&fz->free_blocks);
	return -ENOMEM;
}

/**
 *flush_all_cache_blocks - flush all cache blocks. 
 *Iterate the list of cache blocks and flush it.
 *Wait if a cache block is being used.
 */
static void flush_all_cache_blocks(void)
{
	struct cache_zone *cz = &dev->cache.cache_zone;
	struct cache_block *cb;
	struct cache_block *tmp;

	list_for_each_entry_safe(cb, tmp, &cz->cache_blocks, block_list) {
		while (atomic_read(&cb->ref_cnt) || test_and_set_bit(WRITING_BACK_BIT, &cb->flag)) 
			wait_event(cb->waitq, atomic_read(&cb->ref_cnt) == 0);
		flush_cache_block(cb);
	}
}

/**
 *xcache_destory - destroy the cache.
 *Firstly, flush all cache blocks.Then free all cache blocks volumn.
 */
int xcache_destroy()
{
	flush_all_cache_blocks();
	free_all_cache_blocks(&dev->cache.free_zone.free_blocks);
	return 0;
}
