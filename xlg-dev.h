#ifndef XLG_DEV
#define XLG_DEV

#include "dev-args.h"
#include "xcache-dev.h"
#include "xph-dev.h"

struct xlg_dev {
	int 			size;			/* Device size in sectors */
	short 			users;			/* How many users */
	spinlock_t 		lock;			/* For mutual exclusion */
	struct request_queue 	*queue;			/* The device request queue */
	struct gendisk 		*gd;			/* The gendisk structure */
	struct xcache  		cache;			/* The device cache */
	struct block_device	*bdev;			/* Physical block device */
};

#endif
