/*
 * Compressed RAM block device
 *
 * Copyright (C) 2008, 2009, 2010  Nitin Gupta
 *               2012, 2013 Minchan Kim
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of 3-clause BSD License
 * Released under the terms of GNU General Public License Version 2.0
 *
 */

#ifndef _ZRAM_DRV_H_
#define _ZRAM_DRV_H_

#include <linux/spinlock.h>
#include <linux/zsmalloc.h>

#include "zcomp.h"

/*
 * Some arbitrary value. This is just to catch
 * invalid value for num_devices module parameter.
 */
static const unsigned max_num_devices = 32;

/*-- Configurable parameters */

<<<<<<< HEAD
/* Default zram disk size: 50% of total RAM */
static const unsigned default_disksize_perc_ram = 50;
/* Is totalram_pages less than SUPPOSED_TOTALRAM, promote its default size */
#define SUPPOSED_TOTALRAM	0x20000	/* 512MB */

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
/*
 * Pages that compress to size greater than this are stored
 * uncompressed in memory.
 */
static const size_t max_zpage_size = PAGE_SIZE / 4 * 3;

/*
 * NOTE: max_zpage_size must be less than or equal to:
 *   ZS_MAX_ALLOC_SIZE. Otherwise, zs_malloc() would
 * always return failure.
 */

/*-- End of configurable params */

#define SECTOR_SHIFT		9
#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)
#define ZRAM_LOGICAL_BLOCK_SHIFT 12
#define ZRAM_LOGICAL_BLOCK_SIZE	(1 << ZRAM_LOGICAL_BLOCK_SHIFT)
#define ZRAM_SECTOR_PER_LOGICAL_BLOCK	\
	(1 << (ZRAM_LOGICAL_BLOCK_SHIFT - SECTOR_SHIFT))


/*
 * The lower ZRAM_FLAG_SHIFT bits of table.value is for
 * object size (excluding header), the higher bits is for
 * zram_pageflags.
 *
 * zram is mainly used for memory efficiency so we want to keep memory
 * footprint small so we can squeeze size and flags into a field.
 * The lower ZRAM_FLAG_SHIFT bits is for object size (excluding header),
 * the higher bits is for zram_pageflags.
 */
#define ZRAM_FLAG_SHIFT 24

/* Flags for zram pages (table[page_no].value) */
<<<<<<< HEAD
#ifdef CONFIG_ZSM
enum zram_pageflags {
	/* Page consists entirely of zeros */
	ZRAM_FIRST_NODE ,
	ZRAM_RB_NODE,
	ZRAM_ZSM_NODE,
	ZRAM_ZSM_DONE_NODE,
	ZRAM_ZERO = ZRAM_FLAG_SHIFT + 1,
	ZRAM_ACCESS,	/* page in now accessed */
	__NR_ZRAM_PAGEFLAGS,
};
#else
enum zram_pageflags {
	/* Page consists entirely of zeros */
	ZRAM_ZERO = ZRAM_FLAG_SHIFT + 1,
	ZRAM_ACCESS,	/* page in now accessed */
	__NR_ZRAM_PAGEFLAGS,
};
#endif
=======
enum zram_pageflags {
	/* Page consists entirely of zeros */
	ZRAM_ZERO = ZRAM_FLAG_SHIFT,
	ZRAM_ACCESS,	/* page is now accessed */

	__NR_ZRAM_PAGEFLAGS,
};
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

/*-- Data structures */

/* Allocated for each disk page */
<<<<<<< HEAD
#ifdef CONFIG_ZSM
struct zram_table_entry {
	unsigned long handle;
	unsigned long value;
	struct rb_node node;
	struct list_head head;
	u32 copy_count;
	u32 next_index;
	u32 copy_index;
	u32 checksum;
	u8 flags;
};
#else
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
struct zram_table_entry {
	unsigned long handle;
	unsigned long value;
};
<<<<<<< HEAD
#endif
=======

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
struct zram_stats {
	atomic64_t compr_data_size;	/* compressed size of pages stored */
	atomic64_t num_reads;	/* failed + successful */
	atomic64_t num_writes;	/* --do-- */
<<<<<<< HEAD
=======
	atomic64_t num_migrated;	/* no. of migrated object */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	atomic64_t failed_reads;	/* can happen when memory is too low */
	atomic64_t failed_writes;	/* can happen when memory is too low */
	atomic64_t invalid_io;	/* non-page-aligned I/O requests */
	atomic64_t notify_free;	/* no. of swap slot free notifications */
	atomic64_t zero_pages;		/* no. of zero filled pages */
	atomic64_t pages_stored;	/* no. of pages currently stored */
	atomic_long_t max_used_pages;	/* no. of maximum pages stored */
<<<<<<< HEAD
#ifdef CONFIG_ZSM
	atomic64_t zsm_saved;          /* saved physical size*/
	atomic64_t zsm_saved4k;
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
};

struct zram_meta {
	struct zram_table_entry *table;
	struct zs_pool *mem_pool;
};

struct zram {
	struct zram_meta *meta;
<<<<<<< HEAD
	struct request_queue *queue;
	struct gendisk *disk;
	struct zcomp *comp;

	/* Prevent concurrent execution of device init, reset and R/W request */
	struct rw_semaphore init_lock;
	/*
	 * This is the limit on amount of *uncompressed* worth of data
	 * we can store in a disk.
	 */
	u64 disksize;	/* bytes */
	int max_comp_streams;
	struct zram_stats stats;
	/*
	 * the number of pages zram can consume for storing compressed data
	 */
	unsigned long limit_pages;

	char compressor[10];
};

/* mlog */
unsigned long zram_mlog(void);

=======
	struct zcomp *comp;
	struct gendisk *disk;
	/* Prevent concurrent execution of device init */
	struct rw_semaphore init_lock;
	/*
	 * the number of pages zram can consume for storing compressed data
	 */
	unsigned long limit_pages;
	int max_comp_streams;

	struct zram_stats stats;
	atomic_t refcount; /* refcount for zram_meta */
	/* wait all IO under all of cpu are done */
	wait_queue_head_t io_done;
	/*
	 * This is the limit on amount of *uncompressed* worth of data
	 * we can store in a disk.
	 */
	u64 disksize;	/* bytes */
	char compressor[10];
};
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif
