#ifndef XLG_DEV
#define XLG_DEV

#include "dev-args.h"
#include "xcache-dev.h"
#include "xph-dev.h"

struct xlg_dev {
	int 			size;			/* Device size in sectors */
	//u8 *data;					/* The data array */
	short 			users;			/* How many users */
	//short media_change;				/* Flag a mdia change? */
	spinlock_t 		lock;			/* For mutual exclusion */
	struct request_queue 	*queue;			/* The device request queue */
	struct gendisk 		*gd;			/* The gendisk structure */
	struct xcache  		cache;			/* The device cache */
	//struct timer_list timer;			/* For simulated media change */
	struct block_device	*bdev;			/* Physical block device */
};

#endif
