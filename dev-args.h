#ifndef DEV_ARGS
#define DEV_ARGS

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>					/* everything... */
#include <linux/kernel.h>				/* printk() */
#include <linux/slab.h>					/* kmalloc() */
#include <linux/errno.h>				/* error codes */
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>				/* ioctl() */
#include "xlg_assert.h"
#include <linux/spinlock.h>

#define CACHE_VOL		(100*1024*1024)		/* Cache volumn */
#define PH_VOL			(20*CACHE_VOL)		/* Physical disk volumn */
#define CBLOCK_SIZE		(4096)			/* Cache block size */
#define NBLOCKS			(CACHE_VOL/CBLOCK_SIZE)	/* Cache blocks num */
#define HARDSECT_SIZE		(512)			/* Hard disk sector size */
#define NSECTORS		(PH_VOL/HARDSECT_SIZE)	/* Hard disk sector num */
#define PHD_NAME		"/dev/sdb"		/* physical disk name */
#endif
