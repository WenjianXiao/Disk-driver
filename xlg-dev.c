/*
 * WRITEN TIME 	:  	May 22 2014
 * AUTHOR	:	Xiao Wenjian
 * DESCRIPTION	:	A disk driver with disk cache
 * FUNCTION	:	Complete the logical device layer
 */

#include "xlg-dev.h"

MODULE_LICENSE("Dual BSD/GPL");

static int xlg_major = 0;				/* major device number, 0 means getting a new number */
static int hardsect_size = HARDSECT_SIZE;		/* device sector size */
static int xlg_first_minor = 0;				/* first minor device number */
static const char *xd_name = "xlg_dev";			/* device name */
static int nsectors = NSECTORS;				/* sector num of the device */

#define XLG_MINORS 		1			/* num of the device minor device number */
#define KERNEL_SECTOR_SIZE	512			/* sector size to the kernel */

typedef struct request_queue request_queue_t; 		/* 2.6.32 cancle it, so add it here; */

struct xlg_dev *dev;					/* the logical device */

/**
 * Open the device 
 */
static int xlg_open(struct block_device *bd, fmode_t mode)
{
	printk(KERN_ALERT "------>xlg_open()");
	return 0;

//	struct xlg_dev *dev;
//	ASSERT(bd, bd->bd_disk, bd->bd_disk->private_data);
//	dev = bd->bd_disk->private_data;
//	DOOR(bd, bd->bd_disk, bd->bd_disk->private_data);
//	spin_lock(&dev->lock);
//	dev->users++;
//	spin_unlock(&dev->lock);
//	return 0;
}

/**
 * Close the device
 */
static int xlg_release(struct gendisk *gend, fmode_t mode)
{
	printk(KERN_ALERT "------>xlg_release");
	return 0;
//	struct xlg_dev *dev;
//	ASSERT(gend, gend->private_data);
//	dev = gend->private_data;
//	spin_lock(&dev->lock);
//	dev->users--;
//	spin_unlock(&dev->lock);
//	return 0;
}

/*
 *The ioctl() implementation
 */
static int xlg_ioctl(struct block_device *bd, fmode_t mode,
		     unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;
	struct xlg_dev *dev;

	ASSERT(bd, bd->bd_disk, bd->bd_disk->private_data);
	dev = bd->bd_disk->private_data;

	switch (cmd) {
		case HDIO_GETGEO:
		/*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible.  So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
		size = dev->size*(hardsect_size/KERNEL_SECTOR_SIZE);
		geo.cylinders = (size & ~0x3f) >> 6;		/*  >>6 means having heads*sectors=4*16 sectors per cylinders */
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = 4;
		if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}
	return -ENOTTY;		/* Unknown command */
}

/*
 * The block_device operations fs/blkdev.h
 */
static struct block_device_operations xlg_ops = {
	.owner		= THIS_MODULE,
	.open		= xlg_open,
	.release	= xlg_release,
	.ioctl		= xlg_ioctl
};

/*
 *xlg_make_request - the function of constructing request
 *It's according to not using the request queue.
 */
static int xlg_make_request(request_queue_t *q, struct bio *bio)
{
	int ret = 0;
	struct bio_vec *bv = NULL;
	int i = 0;
	sector_t sector = bio->bi_sector & ~7;
	
	bio_for_each_segment(bv, bio, i) {
		ret = xcache_xfer(bv, sector, bio_data_dir(bio));
		if (ret) 
			break;	
		sector += 8;
	}
	bio_endio(bio, ret);
	return 0;
}

/*
 * setup_device: set up the device structure
 */
int setup_device(void)
{
	int ret;

	/*
	 * Initialize the device structure
	 */
	memset (dev, 0, sizeof (struct xlg_dev));
	dev->size = nsectors;
	spin_lock_init(&dev->lock);

	/*
	 * Use our own make_request function
	 */
	dev->queue = blk_alloc_queue(GFP_KERNEL);
	if (dev->queue == NULL) {
		ret = -ENOMEM;
		goto out_vfree;
	}
	blk_queue_make_request(dev->queue, xlg_make_request);
	blk_queue_logical_block_size(dev->queue, hardsect_size);

	/*
	 *Allocate the gendisk structure.
	 */
	dev->gd = alloc_disk(XLG_MINORS);
	if (! dev->gd) {
		ret = -ENOMEM;
		goto out_vfree;
	}
	dev->gd->major = xlg_major;
	dev->gd->first_minor = xlg_first_minor;
	dev->gd->fops = &xlg_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, xd_name);
	set_capacity(dev->gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
	return 0;
out_vfree:
	return ret;
}

/*
 *xlg_init: initialize our driver module;Called when loading the module;
 */
static int __init xlg_init(void)
{
	int ret = 0;

	/*
	 * Get registered.
	 */
	xlg_major = register_blkdev(xlg_major, xd_name);
	if (xlg_major <= 0) {
		return -EBUSY;
	}
	
	/*
	 *Allocate the device and initialize it
	 */
	dev = kmalloc(sizeof(struct xlg_dev), GFP_KERNEL);
	ASSERT(dev);
	if (dev == NULL) {
		goto xlg_dev_init_failed;
	}

	ret = setup_device();
	if (ret)
		goto xlg_dev_init_failed;
	if (IS_ERR(xph_init())) {
		ret = -ENODEV;
		goto xlg_dev_init_failed;
	}
	ret = xcache_init();
	if (ret) 
		goto xcache_dev_init_failed;
	
	ASSERT(dev->gd);
	add_disk(dev->gd);
	return ret;

xcache_dev_init_failed:
	xcache_destroy();
xlg_dev_init_failed:
	unregister_blkdev(xlg_major, xd_name);
	return ret;
}

/*
 *xlg_exit: Called when removing the driver moduel;
 */
static void __exit xlg_exit(void)
{
	if (dev->gd) {
		del_gendisk(dev->gd);
	}
	if (dev->queue) {
		blk_put_queue(dev->queue);
	}
	xcache_destroy();
	xph_destroy();
	unregister_blkdev(xlg_major, xd_name);
	kfree(dev);
}

module_init(xlg_init);
module_exit(xlg_exit);
MODULE_AUTHOR("xiaowenjian");
MODULE_DESCRIPTION("The disk driver with cache");
MODULE_VERSION("0.1");
