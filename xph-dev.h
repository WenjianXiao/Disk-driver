#ifndef XPH_DEV
#define XPH_DEV

#include "dev-args.h"
#include "xlg-dev.h"
#include "xcache-dev.h"

#define xph_bio_write_block(cb)	xph_bio_xfer(WRITE, cb)
#define xph_bio_read_block(cb)	xph_bio_xfer(READ, cb)

int xph_bio_xfer(int dir, struct cache_block *cb);

struct block_device *xph_init(void);

void xph_destroy(void);

#endif
