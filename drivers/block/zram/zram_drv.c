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

#define KMSG_COMPONENT "zram"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#ifdef CONFIG_ZRAM_DEBUG
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/slab.h>
<<<<<<< HEAD
#include <linux/lzo.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#ifdef CONFIG_ZSM
#include <linux/rbtree.h>
#include <linux/time.h>
#endif
=======
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/err.h>

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#include "zram_drv.h"

/* Globals */
static int zram_major;
static struct zram *zram_devices;
static const char *default_compressor = "lzo";

/* Module params (documentation at end) */
static unsigned int num_devices = 1;
<<<<<<< HEAD
#ifdef CONFIG_ZSM
#define SIZE_MASK (BIT(ZRAM_FLAG_SHIFT) - 1)
#define TABLE_GET_SIZE(X) (X & SIZE_MASK)
static struct rb_root root_zram_tree = RB_ROOT;
static struct rb_root root_zram_tree_4k = RB_ROOT;
spinlock_t zram_node_mutex;
spinlock_t zram_node4k_mutex;

static int zram_test_flag(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag);

static void zram_set_flag(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag);

static void zram_clear_flag(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag);

static int zsm_test_flag(struct zram_meta *meta, struct zram_table_entry *node,
			enum zram_pageflags flag)
{
	return node->flags & BIT(flag);
}

static void zsm_set_flag(struct zram_meta *meta, struct zram_table_entry *node,
			enum zram_pageflags flag)
{
	node->flags |= BIT(flag);
}
static int zsm_test_flag_index(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag)
{
	return meta->table[index].flags & BIT(flag);
}

static void zsm_set_flag_index(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag)
{
	meta->table[index].flags |= BIT(flag);
}

static void zsm_clear_flag_index(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag)
{
	meta->table[index].flags &= ~BIT(flag);
}
static struct zram_table_entry *search_node_in_zram_list(struct zram *zram, struct zram_meta *meta,
	struct zram_table_entry *input_node, struct zram_table_entry *found_node, unsigned char *match_content,
	u32 clen)
{
	struct list_head *list_node = NULL;
	struct zram_table_entry *current_node = NULL;
	unsigned char *cmem;
	int one_node_in_list = 0;
	int compare_count = 0;
	int ret;

	list_node = found_node->head.next;
	if (list_node == &(found_node->head))
		one_node_in_list = 1;
	while ((list_node != &(found_node->head)) || one_node_in_list) {
		one_node_in_list = 0;
		current_node  = list_entry(list_node, struct zram_table_entry, head);
		if ((clen != TABLE_GET_SIZE(current_node->value)) || !zsm_test_flag(meta, current_node,
			ZRAM_FIRST_NODE)) {
			list_node = list_node->next;
		} else {
			if (zsm_test_flag(meta, current_node, ZRAM_ZSM_DONE_NODE) && (current_node->handle != 0)) {
				cmem = zs_map_object(meta->mem_pool, current_node->handle, ZS_MM_RO);
				ret = memcmp(cmem, match_content, TABLE_GET_SIZE(input_node->value));
				compare_count++;
				if (ret == 0) {
					zs_unmap_object(meta->mem_pool, current_node->handle);
					return current_node;
				}
				list_node = list_node->next;
				zs_unmap_object(meta->mem_pool, current_node->handle);
			} else {
				pr_warn("[ZSM] current node is not ready %x and handle is %lx\n",
					current_node->copy_index, current_node->handle);
				list_node = list_node->next;
			}
		}
	}
	return NULL;
}
static struct zram_table_entry *search_node_in_zram_tree(struct zram_table_entry *input_node,
		struct rb_node **parent_node, struct rb_node ***new_node, unsigned char *match_content,
		struct rb_root *local_root_zram_tree)
{
	struct rb_node **new = &(local_root_zram_tree->rb_node);
	struct zram_table_entry *current_node = NULL;
	struct rb_node *parent = NULL;

	current_node = rb_entry(*new, struct zram_table_entry, node);
	if (input_node == NULL) {
		pr_err("[zram][search_node_in_zram_tree] input_node is NULL\n");
		return NULL;
	}
	if (current_node == NULL) {
		*new_node = new;
		*parent_node = NULL;
		return NULL;
	}

	while (*new) {
		current_node = rb_entry(*new, struct zram_table_entry, node);
		parent = *new;
		if (input_node->checksum > current_node->checksum) {
			new = &parent->rb_right;
		} else if (input_node->checksum < current_node->checksum) {
			new = &parent->rb_left;
		} else {
			if (TABLE_GET_SIZE(input_node->value) > TABLE_GET_SIZE(current_node->value))
				new = &parent->rb_right;
			else if (TABLE_GET_SIZE(input_node->value) < TABLE_GET_SIZE(current_node->value))
				new = &parent->rb_left;
			else
				return current_node;
		}
	}
	*parent_node = parent;
	*new_node = new;
	return NULL;
}
static u32 insert_node_to_zram_tree(struct zram *zram, struct zram_meta *meta, u32 index, unsigned char *match_content,
		struct rb_root *local_root_zram_tree, u32 clen)
{
	struct zram_table_entry *current_node = NULL;
	struct zram_table_entry *node_in_list = NULL;
	struct rb_node *parent = NULL;
	struct rb_node **new = NULL;
	struct zram_table_entry *input_node;

	input_node = &(meta->table[index]);
	zsm_set_flag_index(meta, index, ZRAM_ZSM_NODE);
	current_node = search_node_in_zram_tree(input_node, &parent, &new, match_content, local_root_zram_tree);

	/* found node in zram_tree */
	if (NULL != current_node) {
		if (!zsm_test_flag(meta, current_node, ZRAM_RB_NODE)) {
			pr_err("[ZRAM]ERROR !!found wrong rb node 0x%p\n", (void *)current_node);
			BUG_ON(1);
		}

		/* check if there is any other node in this position. */
		node_in_list = search_node_in_zram_list(zram, meta, input_node, current_node, match_content, clen);

		/* found the same node in list */
		if (NULL != node_in_list) {
			/* insert node after the found node */
			if (!zsm_test_flag(meta, current_node, ZRAM_FIRST_NODE)) {
				pr_err("[ZRAM]ERROR !!found wrong first node 0x%p\n", (void *)node_in_list);
				BUG_ON(1);
			}
			input_node->next_index = node_in_list->next_index;
			node_in_list->next_index = index;
			input_node->copy_index = node_in_list->copy_index;

			/* found the same node and add ref count */
			node_in_list->copy_count++;
			if (unlikely(TABLE_GET_SIZE(input_node->value) > max_zpage_size))
				atomic64_add((u64)TABLE_GET_SIZE(input_node->value), &zram->stats.zsm_saved4k);
			else
				atomic64_add((u64)TABLE_GET_SIZE(input_node->value), &zram->stats.zsm_saved);
			input_node->handle = node_in_list->handle;
			list_add(&input_node->head, &node_in_list->head);
			zsm_set_flag_index(meta, index, ZRAM_ZSM_DONE_NODE);
			return 1;
		}
		/* can't found node in list */
		zsm_set_flag_index(meta, index, ZRAM_FIRST_NODE);
		list_add(&input_node->head, &current_node->head);
	} else {
		/* insert node into rb tree */
		zsm_set_flag_index(meta, index, ZRAM_FIRST_NODE);
		zsm_set_flag_index(meta, index, ZRAM_RB_NODE);
		rb_link_node(&(meta->table[index].node), parent, new);
		rb_insert_color(&(meta->table[index].node), local_root_zram_tree);
	}
	return 0;
}
static int remove_node_from_zram_list(struct zram *zram, struct zram_meta *meta, u32 index)
{
	u32 next_index = 0xffffffff;
	u32 pre_index = 0xffffffff;
	u32 current_index = 0xffffffff;
	u32 copy_index = 0xffffffff;
	u32 i = 0;

	next_index  = meta->table[index].next_index;
	list_del(&(meta->table[index].head));
	/* check if there is the same content in list */
	if (index != next_index) {/* found the same page content */
		if (zsm_test_flag_index(meta, index, ZRAM_FIRST_NODE)) {/* delete the fist node of content */
			if (meta->table[index].copy_count <= 0) {
				pr_err("[ZRAM]ERROR !!count < 0\n ");
				BUG_ON(1);
				return 1;
			}
			current_index = meta->table[next_index].next_index;
			meta->table[next_index].copy_index = next_index;
			pre_index = next_index;
			while (current_index != index) {
				i++;
				if (i >= 4096 && (i%1000 == 0)) {
					if (i > meta->table[index].copy_count) {
						BUG_ON(1);
						break;
					}
				}
				meta->table[current_index].copy_index = next_index;
				pre_index = current_index;
				current_index = meta->table[current_index].next_index;
			}
			meta->table[pre_index].next_index = meta->table[index].next_index;
			meta->table[next_index].copy_count = meta->table[index].copy_count - 1;
			zsm_clear_flag_index(meta, index, ZRAM_FIRST_NODE);
			zsm_set_flag_index(meta, next_index, ZRAM_FIRST_NODE);
		} else {
			current_index  = meta->table[index].copy_index;
			pre_index = current_index;
			current_index = meta->table[current_index].next_index;
			while (index != current_index) {
				i++;
				if (i >= 4096 && (i%1000 == 0)) {
					u32 tmp_index = 0;

					tmp_index = meta->table[current_index].copy_index;
					if (i > meta->table[tmp_index].copy_count) {
						BUG_ON(1);
						break;
					}
				}
				pre_index = current_index;
				current_index = meta->table[current_index].next_index;
			}
			meta->table[pre_index].next_index = meta->table[index].next_index;
			copy_index = meta->table[index].copy_index;
			meta->table[copy_index].copy_count = meta->table[copy_index].copy_count - 1;
		}
		if (unlikely(TABLE_GET_SIZE(meta->table[index].value) > max_zpage_size))
			atomic64_sub((u64)TABLE_GET_SIZE(meta->table[index].value), &zram->stats.zsm_saved4k);
		else
			atomic64_sub((u64)TABLE_GET_SIZE(meta->table[index].value), &zram->stats.zsm_saved);
		return 1;
	}
	/* can't found the same page content */
	if (zsm_test_flag_index(meta, index, ZRAM_FIRST_NODE)) {
		zsm_clear_flag_index(meta, index, ZRAM_FIRST_NODE);
	} else {
		pr_err("[ZRAM]ERROR!index != next_index, flag != ZRAM_FIRST_NODE index %x\n ", index);
		BUG_ON(1);
	}
	if (meta->table[index].copy_count != 0) {
		pr_err("[ZRAM]ERROR !!index != next_index and count != 0 index %x\n ", index);
		BUG_ON(1);
	}
	return 0;
}
static int remove_node_from_zram_tree(struct zram *zram, struct zram_meta *meta, u32 index,
		struct rb_root *local_root_zram_tree)
{
	int ret;

	if (zsm_test_flag_index(meta, index, ZRAM_ZSM_NODE)) {
		zsm_clear_flag_index(meta, index, ZRAM_ZSM_NODE);
	} else {
		pr_err("[ZSM] index %x is not belongs to zsm node\n", index);
		BUG_ON(1);
	}
	if (zsm_test_flag_index(meta, index, ZRAM_ZSM_DONE_NODE))
		zsm_clear_flag_index(meta, index, ZRAM_ZSM_DONE_NODE);
	else
		pr_err("[ZSM] index node %x is not set and will be removed\n", index);
	/* if it is rb node, choose other node from list and replace original node. */
	if (zsm_test_flag_index(meta, index, ZRAM_RB_NODE)) {
		zsm_clear_flag_index(meta, index, ZRAM_RB_NODE);
		/* found next node in list */
		if (&(meta->table[index].head) != meta->table[index].head.next) {
			struct zram_table_entry *next_table;

			next_table = list_entry(meta->table[index].head.next, struct zram_table_entry, head);
			rb_replace_node(&(meta->table[index].node), &(next_table->node), local_root_zram_tree);
			zsm_set_flag(meta, next_table, ZRAM_RB_NODE);
			ret = remove_node_from_zram_list(zram, meta, index);
			return ret;
		}
		/* if no other node can be found in list just remove node from rb tree and free handle */
		if (zsm_test_flag_index(meta, index, ZRAM_FIRST_NODE)) {
			zsm_clear_flag_index(meta, index, ZRAM_FIRST_NODE);
		} else {
			pr_err("[ZRAM]ERROR !!ZRAM_RB_NODR's flag != ZRAM_FIRST_NODE index %x\n ",
					index);
			BUG_ON(1);
		}
		rb_erase(&(meta->table[index].node), local_root_zram_tree);
		return 0;
	}
	ret = remove_node_from_zram_list(zram, meta, index);
	return ret;
}
#endif


#define ZRAM_ATTR_RO(name)						\
static ssize_t zram_attr_##name##_show(struct device *d,		\
				struct device_attribute *attr, char *b)	\
{									\
	struct zram *zram = dev_to_zram(d);				\
	return scnprintf(b, PAGE_SIZE, "%llu\n",			\
		(u64)atomic64_read(&zram->stats.name));			\
}									\
static struct device_attribute dev_attr_##name =			\
	__ATTR(name, S_IRUGO, zram_attr_##name##_show, NULL)

static inline int init_done(struct zram *zram)
{
	return zram->meta != NULL;
=======

static inline void deprecated_attr_warn(const char *name)
{
	pr_warn_once("%d (%s) Attribute %s (and others) will be removed. %s\n",
			task_pid_nr(current),
			current->comm,
			name,
			"See zram documentation.");
}

#define ZRAM_ATTR_RO(name)						\
static ssize_t name##_show(struct device *d,		\
				struct device_attribute *attr, char *b)	\
{									\
	struct zram *zram = dev_to_zram(d);				\
									\
	deprecated_attr_warn(__stringify(name));			\
	return scnprintf(b, PAGE_SIZE, "%llu\n",			\
		(u64)atomic64_read(&zram->stats.name));			\
}									\
static DEVICE_ATTR_RO(name);

static inline bool init_done(struct zram *zram)
{
	return zram->disksize;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

static inline struct zram *dev_to_zram(struct device *dev)
{
	return (struct zram *)dev_to_disk(dev)->private_data;
}

<<<<<<< HEAD
=======
static ssize_t compact_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long nr_migrated;
	struct zram *zram = dev_to_zram(dev);
	struct zram_meta *meta;

	down_read(&zram->init_lock);
	if (!init_done(zram)) {
		up_read(&zram->init_lock);
		return -EINVAL;
	}

	meta = zram->meta;
	nr_migrated = zs_compact(meta->mem_pool);
	atomic64_add(nr_migrated, &zram->stats.num_migrated);
	up_read(&zram->init_lock);

	return len;
}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
static ssize_t disksize_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zram *zram = dev_to_zram(dev);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", zram->disksize);
}

static ssize_t initstate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 val;
	struct zram *zram = dev_to_zram(dev);

	down_read(&zram->init_lock);
	val = init_done(zram);
	up_read(&zram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%u\n", val);
}

static ssize_t orig_data_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zram *zram = dev_to_zram(dev);

<<<<<<< HEAD
=======
	deprecated_attr_warn("orig_data_size");
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return scnprintf(buf, PAGE_SIZE, "%llu\n",
		(u64)(atomic64_read(&zram->stats.pages_stored)) << PAGE_SHIFT);
}

static ssize_t mem_used_total_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u64 val = 0;
	struct zram *zram = dev_to_zram(dev);

<<<<<<< HEAD
	down_read(&zram->init_lock);
	if (init_done(zram)) {
		struct zram_meta *meta = zram->meta;

=======
	deprecated_attr_warn("mem_used_total");
	down_read(&zram->init_lock);
	if (init_done(zram)) {
		struct zram_meta *meta = zram->meta;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		val = zs_get_total_pages(meta->mem_pool);
	}
	up_read(&zram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", val << PAGE_SHIFT);
}

static ssize_t max_comp_streams_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	struct zram *zram = dev_to_zram(dev);

	down_read(&zram->init_lock);
	val = zram->max_comp_streams;
	up_read(&zram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t mem_limit_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u64 val;
	struct zram *zram = dev_to_zram(dev);

<<<<<<< HEAD
=======
	deprecated_attr_warn("mem_limit");
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	down_read(&zram->init_lock);
	val = zram->limit_pages;
	up_read(&zram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", val << PAGE_SHIFT);
}

static ssize_t mem_limit_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	u64 limit;
	char *tmp;
	struct zram *zram = dev_to_zram(dev);

	limit = memparse(buf, &tmp);
	if (buf == tmp) /* no chars parsed, invalid input */
		return -EINVAL;

	down_write(&zram->init_lock);
	zram->limit_pages = PAGE_ALIGN(limit) >> PAGE_SHIFT;
	up_write(&zram->init_lock);

	return len;
}

static ssize_t mem_used_max_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u64 val = 0;
	struct zram *zram = dev_to_zram(dev);

<<<<<<< HEAD
=======
	deprecated_attr_warn("mem_used_max");
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	down_read(&zram->init_lock);
	if (init_done(zram))
		val = atomic_long_read(&zram->stats.max_used_pages);
	up_read(&zram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", val << PAGE_SHIFT);
}

static ssize_t mem_used_max_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int err;
	unsigned long val;
	struct zram *zram = dev_to_zram(dev);

	err = kstrtoul(buf, 10, &val);
	if (err || val != 0)
		return -EINVAL;

	down_read(&zram->init_lock);
	if (init_done(zram)) {
		struct zram_meta *meta = zram->meta;
<<<<<<< HEAD

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		atomic_long_set(&zram->stats.max_used_pages,
				zs_get_total_pages(meta->mem_pool));
	}
	up_read(&zram->init_lock);

	return len;
}

static ssize_t max_comp_streams_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int num;
	struct zram *zram = dev_to_zram(dev);
	int ret;

	ret = kstrtoint(buf, 0, &num);
	if (ret < 0)
		return ret;
	if (num < 1)
		return -EINVAL;

	down_write(&zram->init_lock);
	if (init_done(zram)) {
		if (!zcomp_set_max_streams(zram->comp, num)) {
			pr_info("Cannot change max compression streams\n");
			ret = -EINVAL;
			goto out;
		}
	}

	zram->max_comp_streams = num;
	ret = len;
out:
	up_write(&zram->init_lock);
	return ret;
}

static ssize_t comp_algorithm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t sz;
	struct zram *zram = dev_to_zram(dev);

	down_read(&zram->init_lock);
	sz = zcomp_available_show(zram->compressor, buf);
	up_read(&zram->init_lock);

	return sz;
}

static ssize_t comp_algorithm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	struct zram *zram = dev_to_zram(dev);
<<<<<<< HEAD

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	down_write(&zram->init_lock);
	if (init_done(zram)) {
		up_write(&zram->init_lock);
		pr_info("Can't change algorithm for initialized device\n");
		return -EBUSY;
	}
	strlcpy(zram->compressor, buf, sizeof(zram->compressor));
	up_write(&zram->init_lock);
	return len;
}

/* flag operations needs meta->tb_lock */
static int zram_test_flag(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag)
{
	return meta->table[index].value & BIT(flag);
}

static void zram_set_flag(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag)
{
	meta->table[index].value |= BIT(flag);
}

static void zram_clear_flag(struct zram_meta *meta, u32 index,
			enum zram_pageflags flag)
{
	meta->table[index].value &= ~BIT(flag);
}

static size_t zram_get_obj_size(struct zram_meta *meta, u32 index)
{
	return meta->table[index].value & (BIT(ZRAM_FLAG_SHIFT) - 1);
}

static void zram_set_obj_size(struct zram_meta *meta,
					u32 index, size_t size)
{
	unsigned long flags = meta->table[index].value >> ZRAM_FLAG_SHIFT;

	meta->table[index].value = (flags << ZRAM_FLAG_SHIFT) | size;
}

static inline int is_partial_io(struct bio_vec *bvec)
{
	return bvec->bv_len != PAGE_SIZE;
}

/*
 * Check if request is within bounds and aligned on zram logical blocks.
 */
<<<<<<< HEAD
static inline int valid_io_request(struct zram *zram, struct bio *bio)
{
	u64 start, end, bound;

	/* unaligned request */
	if (unlikely(bio->bi_iter.bi_sector &
		     (ZRAM_SECTOR_PER_LOGICAL_BLOCK - 1)))
		return 0;
	if (unlikely(bio->bi_iter.bi_size & (ZRAM_LOGICAL_BLOCK_SIZE - 1)))
		return 0;

	start = bio->bi_iter.bi_sector;
	end = start + (bio->bi_iter.bi_size >> SECTOR_SHIFT);
=======
static inline int valid_io_request(struct zram *zram,
		sector_t start, unsigned int size)
{
	u64 end, bound;

	/* unaligned request */
	if (unlikely(start & (ZRAM_SECTOR_PER_LOGICAL_BLOCK - 1)))
		return 0;
	if (unlikely(size & (ZRAM_LOGICAL_BLOCK_SIZE - 1)))
		return 0;

	end = start + (size >> SECTOR_SHIFT);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	bound = zram->disksize >> SECTOR_SHIFT;
	/* out of range range */
	if (unlikely(start >= bound || end > bound || start > end))
		return 0;

	/* I/O request is valid */
	return 1;
}

<<<<<<< HEAD
static void zram_meta_free(struct zram_meta *meta)
{
=======
static void zram_meta_free(struct zram_meta *meta, u64 disksize)
{
	size_t num_pages = disksize >> PAGE_SHIFT;
	size_t index;

	/* Free all pages that are still in this zram device */
	for (index = 0; index < num_pages; index++) {
		unsigned long handle = meta->table[index].handle;

		if (!handle)
			continue;

		zs_free(meta->mem_pool, handle);
	}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	zs_destroy_pool(meta->mem_pool);
	vfree(meta->table);
	kfree(meta);
}

<<<<<<< HEAD
static struct zram_meta *zram_meta_alloc(u64 disksize)
{
	size_t num_pages;
	struct zram_meta *meta = kmalloc(sizeof(*meta), GFP_KERNEL);

	if (!meta)
		goto out;
=======
static struct zram_meta *zram_meta_alloc(int device_id, u64 disksize)
{
	size_t num_pages;
	char pool_name[8];
	struct zram_meta *meta = kmalloc(sizeof(*meta), GFP_KERNEL);

	if (!meta)
		return NULL;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	num_pages = disksize >> PAGE_SHIFT;
	meta->table = vzalloc(num_pages * sizeof(*meta->table));
	if (!meta->table) {
		pr_err("Error allocating zram address table\n");
<<<<<<< HEAD
		goto free_meta;
	}

	meta->mem_pool = zs_create_pool(GFP_NOIO | __GFP_HIGHMEM);
	if (!meta->mem_pool) {
		pr_err("Error creating memory pool\n");
		goto free_table;
=======
		goto out_error;
	}

	snprintf(pool_name, sizeof(pool_name), "zram%d", device_id);
	meta->mem_pool = zs_create_pool(pool_name, GFP_NOIO | __GFP_HIGHMEM);
	if (!meta->mem_pool) {
		pr_err("Error creating memory pool\n");
		goto out_error;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}

	return meta;

<<<<<<< HEAD
free_table:
	vfree(meta->table);
free_meta:
	kfree(meta);
	meta = NULL;
out:
	return meta;
=======
out_error:
	vfree(meta->table);
	kfree(meta);
	return NULL;
}

static inline bool zram_meta_get(struct zram *zram)
{
	if (atomic_inc_not_zero(&zram->refcount))
		return true;
	return false;
}

static inline void zram_meta_put(struct zram *zram)
{
	atomic_dec(&zram->refcount);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

static void update_position(u32 *index, int *offset, struct bio_vec *bvec)
{
	if (*offset + bvec->bv_len >= PAGE_SIZE)
		(*index)++;
	*offset = (*offset + bvec->bv_len) % PAGE_SIZE;
}

static int page_zero_filled(void *ptr)
{
	unsigned int pos;
	unsigned long *page;

	page = (unsigned long *)ptr;

	for (pos = 0; pos != PAGE_SIZE / sizeof(*page); pos++) {
		if (page[pos])
			return 0;
	}

	return 1;
}

static void handle_zero_page(struct bio_vec *bvec)
{
	struct page *page = bvec->bv_page;
	void *user_mem;

	user_mem = kmap_atomic(page);
	if (is_partial_io(bvec))
		memset(user_mem + bvec->bv_offset, 0, bvec->bv_len);
	else
		clear_page(user_mem);
	kunmap_atomic(user_mem);

	flush_dcache_page(page);
}


/*
 * To protect concurrent access to the same index entry,
 * caller should hold this table index entry's bit_spinlock to
 * indicate this index entry is accessing.
 */
static void zram_free_page(struct zram *zram, size_t index)
{
	struct zram_meta *meta = zram->meta;
	unsigned long handle = meta->table[index].handle;
<<<<<<< HEAD
#ifdef CONFIG_ZSM
	int ret = 0;
#endif
=======

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (unlikely(!handle)) {
		/*
		 * No memory is allocated for zero filled pages.
		 * Simply clear zero page flag.
		 */
		if (zram_test_flag(meta, index, ZRAM_ZERO)) {
			zram_clear_flag(meta, index, ZRAM_ZERO);
			atomic64_dec(&zram->stats.zero_pages);
		}
		return;
	}
<<<<<<< HEAD
#ifdef CONFIG_ZSM
	if (!zram_test_flag(meta, index, ZRAM_ZERO) && zsm_test_flag_index(meta, index, ZRAM_ZSM_NODE)) {
		if (TABLE_GET_SIZE(meta->table[index].value) == PAGE_SIZE) {
			spin_lock(&zram_node4k_mutex);
			ret = remove_node_from_zram_tree(zram, meta, index, &root_zram_tree_4k);
			spin_unlock(&zram_node4k_mutex);
		} else {
			spin_lock(&zram_node_mutex);
			ret = remove_node_from_zram_tree(zram, meta, index, &root_zram_tree);
			spin_unlock(&zram_node_mutex);
		}
	} else if (!zsm_test_flag_index(meta, index, ZRAM_ZSM_NODE))
		pr_err("[ZSM]ERROR! try to free noexist ZSM node index %x\n", index);
	if (ret == 0) {
		zs_free(meta->mem_pool, handle);
		atomic64_sub(zram_get_obj_size(meta, index), &zram->stats.compr_data_size);
		atomic64_dec(&zram->stats.pages_stored);
	}
#else
	zs_free(meta->mem_pool, handle);
	atomic64_sub(zram_get_obj_size(meta, index), &zram->stats.compr_data_size);
	atomic64_dec(&zram->stats.pages_stored);
#endif
=======

	zs_free(meta->mem_pool, handle);

	atomic64_sub(zram_get_obj_size(meta, index),
			&zram->stats.compr_data_size);
	atomic64_dec(&zram->stats.pages_stored);

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	meta->table[index].handle = 0;
	zram_set_obj_size(meta, index, 0);
}

static int zram_decompress_page(struct zram *zram, char *mem, u32 index)
{
	int ret = 0;
	unsigned char *cmem;
	struct zram_meta *meta = zram->meta;
	unsigned long handle;
	size_t size;

	bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
	handle = meta->table[index].handle;
	size = zram_get_obj_size(meta, index);

	if (!handle || zram_test_flag(meta, index, ZRAM_ZERO)) {
		bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);
<<<<<<< HEAD
		clear_page(mem);
=======
		memset(mem, 0, PAGE_SIZE);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		return 0;
	}

	cmem = zs_map_object(meta->mem_pool, handle, ZS_MM_RO);
	if (size == PAGE_SIZE)
<<<<<<< HEAD
		copy_page(mem, cmem);
=======
		memcpy(mem, cmem, PAGE_SIZE);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	else
		ret = zcomp_decompress(zram->comp, cmem, size, mem);
	zs_unmap_object(meta->mem_pool, handle);
	bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);

	/* Should NEVER happen. Return bio error if it does. */
	if (unlikely(ret)) {
		pr_err("Decompression failed! err=%d, page=%u\n", ret, index);
		return ret;
	}

	return 0;
}

static int zram_bvec_read(struct zram *zram, struct bio_vec *bvec,
<<<<<<< HEAD
			  u32 index, int offset, struct bio *bio)
=======
			  u32 index, int offset)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	int ret;
	struct page *page;
	unsigned char *user_mem, *uncmem = NULL;
	struct zram_meta *meta = zram->meta;
<<<<<<< HEAD

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	page = bvec->bv_page;

	bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
	if (unlikely(!meta->table[index].handle) ||
			zram_test_flag(meta, index, ZRAM_ZERO)) {
		bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);
		handle_zero_page(bvec);
		return 0;
	}
	bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);

	if (is_partial_io(bvec))
		/* Use  a temporary buffer to decompress the page */
		uncmem = kmalloc(PAGE_SIZE, GFP_NOIO);

	user_mem = kmap_atomic(page);
	if (!is_partial_io(bvec))
		uncmem = user_mem;

	if (!uncmem) {
		pr_info("Unable to allocate temp memory\n");
		ret = -ENOMEM;
		goto out_cleanup;
	}

	ret = zram_decompress_page(zram, uncmem, index);
	/* Should NEVER happen. Return bio error if it does. */
	if (unlikely(ret))
		goto out_cleanup;

	if (is_partial_io(bvec))
		memcpy(user_mem + bvec->bv_offset, uncmem + offset,
				bvec->bv_len);

	flush_dcache_page(page);
	ret = 0;
out_cleanup:
	kunmap_atomic(user_mem);
	if (is_partial_io(bvec))
		kfree(uncmem);
	return ret;
}

static inline void update_used_max(struct zram *zram,
					const unsigned long pages)
{
<<<<<<< HEAD
	int old_max, cur_max;
=======
	unsigned long old_max, cur_max;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	old_max = atomic_long_read(&zram->stats.max_used_pages);

	do {
		cur_max = old_max;
		if (pages > cur_max)
			old_max = atomic_long_cmpxchg(
				&zram->stats.max_used_pages, cur_max, pages);
	} while (old_max != cur_max);
}

static int zram_bvec_write(struct zram *zram, struct bio_vec *bvec, u32 index,
			   int offset)
{
	int ret = 0;
<<<<<<< HEAD
#ifdef CONFIG_ZSM
	int checksum = 0;
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	size_t clen;
	unsigned long handle;
	struct page *page;
	unsigned char *user_mem, *cmem, *src, *uncmem = NULL;
	struct zram_meta *meta = zram->meta;
	struct zcomp_strm *zstrm;
	bool locked = false;
	unsigned long alloced_pages;

	page = bvec->bv_page;
<<<<<<< HEAD

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (is_partial_io(bvec)) {
		/*
		 * This is a partial IO. We need to read the full page
		 * before to write the changes.
		 */
		uncmem = kmalloc(PAGE_SIZE, GFP_NOIO);
		if (!uncmem) {
			ret = -ENOMEM;
			goto out;
		}
		ret = zram_decompress_page(zram, uncmem, index);
		if (ret)
			goto out;
<<<<<<< HEAD
#ifdef CONFIG_ZSM
		if (!zram_test_flag(meta, index, ZRAM_ZERO)) {
			if (zram_get_obj_size(meta, index) == PAGE_SIZE) {
				spin_lock(&zram_node4k_mutex);
				ret = remove_node_from_zram_tree(zram, meta, index, &root_zram_tree_4k);
				spin_unlock(&zram_node4k_mutex);
			} else {
				spin_lock(&zram_node_mutex);
				ret = remove_node_from_zram_tree(zram, meta, index, &root_zram_tree);
				spin_unlock(&zram_node_mutex);
			}
		}
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}

	zstrm = zcomp_strm_find(zram->comp);
	locked = true;
	user_mem = kmap_atomic(page);

	if (is_partial_io(bvec)) {
		memcpy(uncmem + offset, user_mem + bvec->bv_offset,
		       bvec->bv_len);
		kunmap_atomic(user_mem);
		user_mem = NULL;
	} else {
		uncmem = user_mem;
	}

	if (page_zero_filled(uncmem)) {
		if (user_mem)
			kunmap_atomic(user_mem);
		/* Free memory associated with this sector now. */
		bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
		zram_free_page(zram, index);
		zram_set_flag(meta, index, ZRAM_ZERO);
		bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);

		atomic64_inc(&zram->stats.zero_pages);
		ret = 0;
		goto out;
	}
<<<<<<< HEAD
#ifdef CONFIG_ZSM
	ret = zcomp_compress_zram(zram->comp, zstrm, uncmem, &clen, &checksum);
#else
	ret = zcomp_compress(zram->comp, zstrm, uncmem, &clen);
#endif
=======

	ret = zcomp_compress(zram->comp, zstrm, uncmem, &clen);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (!is_partial_io(bvec)) {
		kunmap_atomic(user_mem);
		user_mem = NULL;
		uncmem = NULL;
	}

	if (unlikely(ret)) {
		pr_err("Compression failed! err=%d\n", ret);
		goto out;
	}
	src = zstrm->buffer;
	if (unlikely(clen > max_zpage_size)) {
		clen = PAGE_SIZE;
		if (is_partial_io(bvec))
			src = uncmem;
<<<<<<< HEAD
#ifdef CONFIG_ZSM
		{
		int search_ret = 0;

		bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
		zram_free_page(zram, index);
		meta->table[index].checksum = checksum;
		zram_set_obj_size(meta, index, clen);
		meta->table[index].next_index = index;
		meta->table[index].copy_index = index;
		meta->table[index].copy_count = 0;
		INIT_LIST_HEAD(&(meta->table[index].head));
		bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);
		if ((clen == PAGE_SIZE) && !is_partial_io(bvec)) {
			src = kmap_atomic(page);
			spin_lock(&zram_node4k_mutex);
			search_ret = insert_node_to_zram_tree(zram, meta, index, src, &root_zram_tree_4k, clen);
			spin_unlock(&zram_node4k_mutex);
			kunmap_atomic(src);
		} else {
			spin_lock(&zram_node4k_mutex);
			search_ret = insert_node_to_zram_tree(zram, meta, index, src, &root_zram_tree_4k, clen);
			spin_unlock(&zram_node4k_mutex);
		}
		if (search_ret) {
			ret = 0;
			goto out;
		}
		}
#endif
	}
#ifdef CONFIG_ZSM
	if (unlikely(clen <= max_zpage_size)) {
		int search_ret = 0;

		bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
		zram_free_page(zram, index);
		meta->table[index].checksum = checksum;
		zram_set_obj_size(meta, index, clen);
		meta->table[index].next_index = index;
		meta->table[index].copy_index = index;
		meta->table[index].copy_count = 0;
		INIT_LIST_HEAD(&(meta->table[index].head));
		bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);
		spin_lock(&zram_node_mutex);
		search_ret = insert_node_to_zram_tree(zram, meta, index, src, &root_zram_tree, clen);
		spin_unlock(&zram_node_mutex);
		if (search_ret) {
			ret = 0;
			goto out;
		}
	}
#endif
=======
	}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	handle = zs_malloc(meta->mem_pool, clen);
	if (!handle) {
		pr_info("Error allocating memory for compressed page: %u, size=%zu\n",
			index, clen);
		ret = -ENOMEM;
		goto out;
	}

	alloced_pages = zs_get_total_pages(meta->mem_pool);
	if (zram->limit_pages && alloced_pages > zram->limit_pages) {
		zs_free(meta->mem_pool, handle);
		ret = -ENOMEM;
		goto out;
	}

	update_used_max(zram, alloced_pages);

	cmem = zs_map_object(meta->mem_pool, handle, ZS_MM_WO);

	if ((clen == PAGE_SIZE) && !is_partial_io(bvec)) {
		src = kmap_atomic(page);
<<<<<<< HEAD
		copy_page(cmem, src);
=======
		memcpy(cmem, src, PAGE_SIZE);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		kunmap_atomic(src);
	} else {
		memcpy(cmem, src, clen);
	}

	zcomp_strm_release(zram->comp, zstrm);
	locked = false;
	zs_unmap_object(meta->mem_pool, handle);

	/*
	 * Free memory associated with this sector
	 * before overwriting unused sectors.
	 */
	bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
	zram_free_page(zram, index);
<<<<<<< HEAD
	meta->table[index].handle = handle;
	zram_set_obj_size(meta, index, clen);
#ifdef CONFIG_ZSM
	zsm_set_flag_index(meta, index, ZRAM_ZSM_DONE_NODE);
#endif
=======

	meta->table[index].handle = handle;
	zram_set_obj_size(meta, index, clen);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);

	/* Update stats */
	atomic64_add(clen, &zram->stats.compr_data_size);
	atomic64_inc(&zram->stats.pages_stored);
out:
	if (locked)
		zcomp_strm_release(zram->comp, zstrm);
	if (is_partial_io(bvec))
		kfree(uncmem);
	return ret;
}

static int zram_bvec_rw(struct zram *zram, struct bio_vec *bvec, u32 index,
<<<<<<< HEAD
			int offset, struct bio *bio)
{
	int ret;
	int rw = bio_data_dir(bio);

	if (rw == READ) {
		atomic64_inc(&zram->stats.num_reads);
		ret = zram_bvec_read(zram, bvec, index, offset, bio);
=======
			int offset, int rw)
{
	int ret;

	if (rw == READ) {
		atomic64_inc(&zram->stats.num_reads);
		ret = zram_bvec_read(zram, bvec, index, offset);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	} else {
		atomic64_inc(&zram->stats.num_writes);
		ret = zram_bvec_write(zram, bvec, index, offset);
	}

	if (unlikely(ret)) {
		if (rw == READ)
			atomic64_inc(&zram->stats.failed_reads);
		else
			atomic64_inc(&zram->stats.failed_writes);
	}

	return ret;
}

/*
 * zram_bio_discard - handler on discard request
 * @index: physical block index in PAGE_SIZE units
 * @offset: byte offset within physical block
 */
static void zram_bio_discard(struct zram *zram, u32 index,
			     int offset, struct bio *bio)
{
	size_t n = bio->bi_iter.bi_size;
	struct zram_meta *meta = zram->meta;

	/*
	 * zram manages data in physical block size units. Because logical block
	 * size isn't identical with physical block size on some arch, we
	 * could get a discard request pointing to a specific offset within a
	 * certain physical block.  Although we can handle this request by
	 * reading that physiclal block and decompressing and partially zeroing
	 * and re-compressing and then re-storing it, this isn't reasonable
	 * because our intent with a discard request is to save memory.  So
	 * skipping this logical block is appropriate here.
	 */
	if (offset) {
		if (n <= (PAGE_SIZE - offset))
			return;

		n -= (PAGE_SIZE - offset);
		index++;
	}

	while (n >= PAGE_SIZE) {
		bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
		zram_free_page(zram, index);
		bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);
		atomic64_inc(&zram->stats.notify_free);
		index++;
		n -= PAGE_SIZE;
	}
}

<<<<<<< HEAD
static void zram_reset_device(struct zram *zram, bool reset_capacity)
{
	size_t index;
	struct zram_meta *meta;
=======
static void zram_reset_device(struct zram *zram)
{
	struct zram_meta *meta;
	struct zcomp *comp;
	u64 disksize;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	down_write(&zram->init_lock);

	zram->limit_pages = 0;

	if (!init_done(zram)) {
		up_write(&zram->init_lock);
		return;
	}

	meta = zram->meta;
<<<<<<< HEAD
	/* Free all pages that are still in this zram device */
	for (index = 0; index < zram->disksize >> PAGE_SHIFT; index++) {
		unsigned long handle = meta->table[index].handle;

		if (!handle)
			continue;

		zs_free(meta->mem_pool, handle);
	}

	zcomp_destroy(zram->comp);
	zram->max_comp_streams = 1;

	zram_meta_free(zram->meta);
	zram->meta = NULL;
	/* Reset stats */
	memset(&zram->stats, 0, sizeof(zram->stats));

	zram->disksize = 0;
	if (reset_capacity)
		set_capacity(zram->disk, 0);

	up_write(&zram->init_lock);

	/*
	 * Revalidate disk out of the init_lock to avoid lockdep splat.
	 * It's okay because disk's capacity is protected by init_lock
	 * so that revalidate_disk always sees up-to-date capacity.
	 */
	if (reset_capacity)
		revalidate_disk(zram->disk);
=======
	comp = zram->comp;
	disksize = zram->disksize;
	/*
	 * Refcount will go down to 0 eventually and r/w handler
	 * cannot handle further I/O so it will bail out by
	 * check zram_meta_get.
	 */
	zram_meta_put(zram);
	/*
	 * We want to free zram_meta in process context to avoid
	 * deadlock between reclaim path and any other locks.
	 */
	wait_event(zram->io_done, atomic_read(&zram->refcount) == 0);

	/* Reset stats */
	memset(&zram->stats, 0, sizeof(zram->stats));
	zram->disksize = 0;
	zram->max_comp_streams = 1;

	set_capacity(zram->disk, 0);
	part_stat_set_all(&zram->disk->part0, 0);

	up_write(&zram->init_lock);
	/* I/O operation under all of CPU are done so let's free */
	zram_meta_free(meta, disksize);
	zcomp_destroy(comp);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

static ssize_t disksize_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	u64 disksize;
	struct zcomp *comp;
	struct zram_meta *meta;
	struct zram *zram = dev_to_zram(dev);
	int err;

	disksize = memparse(buf, NULL);
<<<<<<< HEAD
	if (!disksize) {
		/* Give it a default disksize */
		disksize = default_disksize_perc_ram * ((totalram_pages << PAGE_SHIFT) / 100);
		/* Promote the default disksize if totalram_pages is smaller */
		if (totalram_pages < SUPPOSED_TOTALRAM)
			disksize += (disksize >> 1);
	}

	disksize = PAGE_ALIGN(disksize);
	meta = zram_meta_alloc(disksize);
=======
	if (!disksize)
		return -EINVAL;

	disksize = PAGE_ALIGN(disksize);
	meta = zram_meta_alloc(zram->disk->first_minor, disksize);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (!meta)
		return -ENOMEM;

	comp = zcomp_create(zram->compressor, zram->max_comp_streams);
	if (IS_ERR(comp)) {
		pr_info("Cannot initialise %s compressing backend\n",
				zram->compressor);
		err = PTR_ERR(comp);
		goto out_free_meta;
	}

	down_write(&zram->init_lock);
	if (init_done(zram)) {
		pr_info("Cannot change disksize for initialized device\n");
		err = -EBUSY;
		goto out_destroy_comp;
	}

<<<<<<< HEAD
=======
	init_waitqueue_head(&zram->io_done);
	atomic_set(&zram->refcount, 1);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	zram->meta = meta;
	zram->comp = comp;
	zram->disksize = disksize;
	set_capacity(zram->disk, zram->disksize >> SECTOR_SHIFT);
	up_write(&zram->init_lock);

	/*
	 * Revalidate disk out of the init_lock to avoid lockdep splat.
	 * It's okay because disk's capacity is protected by init_lock
	 * so that revalidate_disk always sees up-to-date capacity.
	 */
	revalidate_disk(zram->disk);

	return len;

out_destroy_comp:
	up_write(&zram->init_lock);
	zcomp_destroy(comp);
out_free_meta:
<<<<<<< HEAD
	zram_meta_free(meta);
=======
	zram_meta_free(meta, disksize);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return err;
}

static ssize_t reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	unsigned short do_reset;
	struct zram *zram;
	struct block_device *bdev;

	zram = dev_to_zram(dev);
	bdev = bdget_disk(zram->disk, 0);

	if (!bdev)
		return -ENOMEM;

<<<<<<< HEAD
	/* Do not reset an active device! */
	if (bdev->bd_holders) {
=======
	mutex_lock(&bdev->bd_mutex);
	/* Do not reset an active device! */
	if (bdev->bd_openers) {
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		ret = -EBUSY;
		goto out;
	}

	ret = kstrtou16(buf, 10, &do_reset);
	if (ret)
		goto out;

	if (!do_reset) {
		ret = -EINVAL;
		goto out;
	}

	/* Make sure all pending I/O is finished */
	fsync_bdev(bdev);
<<<<<<< HEAD
	bdput(bdev);

	zram_reset_device(zram, true);
	return len;

out:
=======
	zram_reset_device(zram);

	mutex_unlock(&bdev->bd_mutex);
	revalidate_disk(zram->disk);
	bdput(bdev);

	return len;

out:
	mutex_unlock(&bdev->bd_mutex);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	bdput(bdev);
	return ret;
}

static void __zram_make_request(struct zram *zram, struct bio *bio)
{
<<<<<<< HEAD
	int offset;
=======
	int offset, rw;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	u32 index;
	struct bio_vec bvec;
	struct bvec_iter iter;

	index = bio->bi_iter.bi_sector >> SECTORS_PER_PAGE_SHIFT;
	offset = (bio->bi_iter.bi_sector &
		  (SECTORS_PER_PAGE - 1)) << SECTOR_SHIFT;

	if (unlikely(bio->bi_rw & REQ_DISCARD)) {
		zram_bio_discard(zram, index, offset, bio);
		bio_endio(bio, 0);
		return;
	}

<<<<<<< HEAD
=======
	rw = bio_data_dir(bio);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	bio_for_each_segment(bvec, bio, iter) {
		int max_transfer_size = PAGE_SIZE - offset;

		if (bvec.bv_len > max_transfer_size) {
			/*
			 * zram_bvec_rw() can only make operation on a single
			 * zram page. Split the bio vector.
			 */
			struct bio_vec bv;

			bv.bv_page = bvec.bv_page;
			bv.bv_len = max_transfer_size;
			bv.bv_offset = bvec.bv_offset;

<<<<<<< HEAD
			if (zram_bvec_rw(zram, &bv, index, offset, bio) < 0)
=======
			if (zram_bvec_rw(zram, &bv, index, offset, rw) < 0)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
				goto out;

			bv.bv_len = bvec.bv_len - max_transfer_size;
			bv.bv_offset += max_transfer_size;
<<<<<<< HEAD
			if (zram_bvec_rw(zram, &bv, index + 1, 0, bio) < 0)
				goto out;
		} else
			if (zram_bvec_rw(zram, &bvec, index, offset, bio) < 0)
=======
			if (zram_bvec_rw(zram, &bv, index + 1, 0, rw) < 0)
				goto out;
		} else
			if (zram_bvec_rw(zram, &bvec, index, offset, rw) < 0)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
				goto out;

		update_position(&index, &offset, &bvec);
	}

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return;

out:
	bio_io_error(bio);
}

/*
 * Handler function for all zram I/O requests.
 */
static void zram_make_request(struct request_queue *queue, struct bio *bio)
{
	struct zram *zram = queue->queuedata;

<<<<<<< HEAD
	down_read(&zram->init_lock);
	if (unlikely(!init_done(zram)))
		goto error;

	if (!valid_io_request(zram, bio)) {
		atomic64_inc(&zram->stats.invalid_io);
		goto error;
	}

	__zram_make_request(zram, bio);
	up_read(&zram->init_lock);

	return;

error:
	up_read(&zram->init_lock);
=======
	if (unlikely(!zram_meta_get(zram)))
		goto error;

	if (!valid_io_request(zram, bio->bi_iter.bi_sector,
					bio->bi_iter.bi_size)) {
		atomic64_inc(&zram->stats.invalid_io);
		goto put_zram;
	}

	__zram_make_request(zram, bio);
	zram_meta_put(zram);
	return;
put_zram:
	zram_meta_put(zram);
error:
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	bio_io_error(bio);
}

static void zram_slot_free_notify(struct block_device *bdev,
				unsigned long index)
{
	struct zram *zram;
	struct zram_meta *meta;

	zram = bdev->bd_disk->private_data;
	meta = zram->meta;

	bit_spin_lock(ZRAM_ACCESS, &meta->table[index].value);
	zram_free_page(zram, index);
	bit_spin_unlock(ZRAM_ACCESS, &meta->table[index].value);
	atomic64_inc(&zram->stats.notify_free);
}

<<<<<<< HEAD
static const struct block_device_operations zram_devops = {
	.swap_slot_free_notify = zram_slot_free_notify,
	.owner = THIS_MODULE
};

static DEVICE_ATTR(disksize, S_IRUGO | S_IWUSR,
		disksize_show, disksize_store);
static DEVICE_ATTR(initstate, S_IRUGO, initstate_show, NULL);
static DEVICE_ATTR(reset, S_IWUSR, NULL, reset_store);
static DEVICE_ATTR(orig_data_size, S_IRUGO, orig_data_size_show, NULL);
static DEVICE_ATTR(mem_used_total, S_IRUGO, mem_used_total_show, NULL);
static DEVICE_ATTR(mem_limit, S_IRUGO | S_IWUSR, mem_limit_show,
		mem_limit_store);
static DEVICE_ATTR(mem_used_max, S_IRUGO | S_IWUSR, mem_used_max_show,
		mem_used_max_store);
static DEVICE_ATTR(max_comp_streams, S_IRUGO | S_IWUSR,
		max_comp_streams_show, max_comp_streams_store);
static DEVICE_ATTR(comp_algorithm, S_IRUGO | S_IWUSR,
		comp_algorithm_show, comp_algorithm_store);

=======
static int zram_rw_page(struct block_device *bdev, sector_t sector,
		       struct page *page, int rw)
{
	int offset, err = -EIO;
	u32 index;
	struct zram *zram;
	struct bio_vec bv;

	zram = bdev->bd_disk->private_data;
	if (unlikely(!zram_meta_get(zram)))
		goto out;

	if (!valid_io_request(zram, sector, PAGE_SIZE)) {
		atomic64_inc(&zram->stats.invalid_io);
		err = -EINVAL;
		goto put_zram;
	}

	index = sector >> SECTORS_PER_PAGE_SHIFT;
	offset = (sector & (SECTORS_PER_PAGE - 1)) << SECTOR_SHIFT;

	bv.bv_page = page;
	bv.bv_len = PAGE_SIZE;
	bv.bv_offset = 0;

	err = zram_bvec_rw(zram, &bv, index, offset, rw);
put_zram:
	zram_meta_put(zram);
out:
	/*
	 * If I/O fails, just return error(ie, non-zero) without
	 * calling page_endio.
	 * It causes resubmit the I/O with bio request by upper functions
	 * of rw_page(e.g., swap_readpage, __swap_writepage) and
	 * bio->bi_end_io does things to handle the error
	 * (e.g., SetPageError, set_page_dirty and extra works).
	 */
	if (err == 0)
		page_endio(page, rw, 0);
	return err;
}

static const struct block_device_operations zram_devops = {
	.swap_slot_free_notify = zram_slot_free_notify,
	.rw_page = zram_rw_page,
	.owner = THIS_MODULE
};

static DEVICE_ATTR_WO(compact);
static DEVICE_ATTR_RW(disksize);
static DEVICE_ATTR_RO(initstate);
static DEVICE_ATTR_WO(reset);
static DEVICE_ATTR_RO(orig_data_size);
static DEVICE_ATTR_RO(mem_used_total);
static DEVICE_ATTR_RW(mem_limit);
static DEVICE_ATTR_RW(mem_used_max);
static DEVICE_ATTR_RW(max_comp_streams);
static DEVICE_ATTR_RW(comp_algorithm);

static ssize_t io_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zram *zram = dev_to_zram(dev);
	ssize_t ret;

	down_read(&zram->init_lock);
	ret = scnprintf(buf, PAGE_SIZE,
			"%8llu %8llu %8llu %8llu\n",
			(u64)atomic64_read(&zram->stats.failed_reads),
			(u64)atomic64_read(&zram->stats.failed_writes),
			(u64)atomic64_read(&zram->stats.invalid_io),
			(u64)atomic64_read(&zram->stats.notify_free));
	up_read(&zram->init_lock);

	return ret;
}

static ssize_t mm_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct zram *zram = dev_to_zram(dev);
	u64 orig_size, mem_used = 0;
	long max_used;
	ssize_t ret;

	down_read(&zram->init_lock);
	if (init_done(zram))
		mem_used = zs_get_total_pages(zram->meta->mem_pool);

	orig_size = atomic64_read(&zram->stats.pages_stored);
	max_used = atomic_long_read(&zram->stats.max_used_pages);

	ret = scnprintf(buf, PAGE_SIZE,
			"%8llu %8llu %8llu %8lu %8ld %8llu %8llu\n",
			orig_size << PAGE_SHIFT,
			(u64)atomic64_read(&zram->stats.compr_data_size),
			mem_used << PAGE_SHIFT,
			zram->limit_pages << PAGE_SHIFT,
			max_used << PAGE_SHIFT,
			(u64)atomic64_read(&zram->stats.zero_pages),
			(u64)atomic64_read(&zram->stats.num_migrated));
	up_read(&zram->init_lock);

	return ret;
}

static DEVICE_ATTR_RO(io_stat);
static DEVICE_ATTR_RO(mm_stat);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
ZRAM_ATTR_RO(num_reads);
ZRAM_ATTR_RO(num_writes);
ZRAM_ATTR_RO(failed_reads);
ZRAM_ATTR_RO(failed_writes);
ZRAM_ATTR_RO(invalid_io);
ZRAM_ATTR_RO(notify_free);
ZRAM_ATTR_RO(zero_pages);
ZRAM_ATTR_RO(compr_data_size);

static struct attribute *zram_disk_attrs[] = {
	&dev_attr_disksize.attr,
	&dev_attr_initstate.attr,
	&dev_attr_reset.attr,
	&dev_attr_num_reads.attr,
	&dev_attr_num_writes.attr,
	&dev_attr_failed_reads.attr,
	&dev_attr_failed_writes.attr,
<<<<<<< HEAD
=======
	&dev_attr_compact.attr,
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	&dev_attr_invalid_io.attr,
	&dev_attr_notify_free.attr,
	&dev_attr_zero_pages.attr,
	&dev_attr_orig_data_size.attr,
	&dev_attr_compr_data_size.attr,
	&dev_attr_mem_used_total.attr,
	&dev_attr_mem_limit.attr,
	&dev_attr_mem_used_max.attr,
	&dev_attr_max_comp_streams.attr,
	&dev_attr_comp_algorithm.attr,
<<<<<<< HEAD
=======
	&dev_attr_io_stat.attr,
	&dev_attr_mm_stat.attr,
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	NULL,
};

static struct attribute_group zram_disk_attr_group = {
	.attrs = zram_disk_attrs,
};

<<<<<<< HEAD
static int create_device(struct zram *zram, int device_id)
{
	int ret = -ENOMEM;

	init_rwsem(&zram->init_lock);
#ifdef CONFIG_ZSM
	spin_lock_init(&zram_node_mutex);
	spin_lock_init(&zram_node4k_mutex);
#endif
	zram->queue = blk_alloc_queue(GFP_KERNEL);
	if (!zram->queue) {
=======
static const struct attribute_group *zram_disk_attr_groups[] = {
	&zram_disk_attr_group,
	NULL,
};

static int create_device(struct zram *zram, int device_id)
{
	struct request_queue *queue;
	int ret = -ENOMEM;

	init_rwsem(&zram->init_lock);

	queue = blk_alloc_queue(GFP_KERNEL);
	if (!queue) {
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		pr_err("Error allocating disk queue for device %d\n",
			device_id);
		goto out;
	}

<<<<<<< HEAD
	blk_queue_make_request(zram->queue, zram_make_request);
	zram->queue->queuedata = zram;
=======
	blk_queue_make_request(queue, zram_make_request);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	 /* gendisk structure */
	zram->disk = alloc_disk(1);
	if (!zram->disk) {
		pr_warn("Error allocating disk structure for device %d\n",
			device_id);
<<<<<<< HEAD
=======
		ret = -ENOMEM;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		goto out_free_queue;
	}

	zram->disk->major = zram_major;
	zram->disk->first_minor = device_id;
	zram->disk->fops = &zram_devops;
<<<<<<< HEAD
	zram->disk->queue = zram->queue;
=======
	zram->disk->queue = queue;
	zram->disk->queue->queuedata = zram;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	zram->disk->private_data = zram;
	snprintf(zram->disk->disk_name, 16, "zram%d", device_id);

	/* Actual capacity set using syfs (/sys/block/zram<id>/disksize */
	set_capacity(zram->disk, 0);
	/* zram devices sort of resembles non-rotational disks */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, zram->disk->queue);
	queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, zram->disk->queue);
	/*
	 * To ensure that we always get PAGE_SIZE aligned
	 * and n*PAGE_SIZED sized I/O requests.
	 */
	blk_queue_physical_block_size(zram->disk->queue, PAGE_SIZE);
	blk_queue_logical_block_size(zram->disk->queue,
					ZRAM_LOGICAL_BLOCK_SIZE);
	blk_queue_io_min(zram->disk->queue, PAGE_SIZE);
	blk_queue_io_opt(zram->disk->queue, PAGE_SIZE);
	zram->disk->queue->limits.discard_granularity = PAGE_SIZE;
	zram->disk->queue->limits.max_discard_sectors = UINT_MAX;
<<<<<<< HEAD
=======
	zram->disk->queue->limits.max_sectors = SECTORS_PER_PAGE;
	zram->disk->queue->limits.chunk_sectors = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	/*
	 * zram_bio_discard() will clear all logical blocks if logical block
	 * size is identical with physical block size(PAGE_SIZE). But if it is
	 * different, we will skip discarding some parts of logical blocks in
	 * the part of the request range which isn't aligned to physical block
	 * size.  So we can't ensure that all discarded logical blocks are
	 * zeroed.
	 */
	if (ZRAM_LOGICAL_BLOCK_SIZE == PAGE_SIZE)
		zram->disk->queue->limits.discard_zeroes_data = 1;
	else
		zram->disk->queue->limits.discard_zeroes_data = 0;
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, zram->disk->queue);

<<<<<<< HEAD
	add_disk(zram->disk);

	ret = sysfs_create_group(&disk_to_dev(zram->disk)->kobj,
				&zram_disk_attr_group);
	if (ret < 0) {
		pr_warn("Error creating sysfs group");
		goto out_free_disk;
	}
=======
	disk_to_dev(zram->disk)->groups = zram_disk_attr_groups;
	add_disk(zram->disk);

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	strlcpy(zram->compressor, default_compressor, sizeof(zram->compressor));
	zram->meta = NULL;
	zram->max_comp_streams = 1;
	return 0;

<<<<<<< HEAD
out_free_disk:
	del_gendisk(zram->disk);
	put_disk(zram->disk);
out_free_queue:
	blk_cleanup_queue(zram->queue);
=======
out_free_queue:
	blk_cleanup_queue(queue);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
out:
	return ret;
}

<<<<<<< HEAD
static void destroy_device(struct zram *zram)
{
	sysfs_remove_group(&disk_to_dev(zram->disk)->kobj,
			&zram_disk_attr_group);

	del_gendisk(zram->disk);
	put_disk(zram->disk);

	blk_cleanup_queue(zram->queue);
}

unsigned long zram_mlog(void)
{
#define P2K(x) (((unsigned long)x) << (PAGE_SHIFT - 10))
	if (num_devices == 1 && init_done(zram_devices))
		return P2K(zs_get_total_pages(zram_devices->meta->mem_pool));
#undef P2K

	return 0;
}

#ifdef CONFIG_PROC_FS
static int zraminfo_proc_show(struct seq_file *m, void *v)
{
	if (num_devices == 1 && init_done(zram_devices)) {
#define P2K(x) (((unsigned long)x) << (PAGE_SHIFT - 10))
#define B2K(x) (((unsigned long)x) >> (10))
		seq_printf(m,
				"DiskSize:       %8lu kB\n"
				"OrigSize:       %8lu kB\n"
				"ComprSize:      %8lu kB\n"
				"MemUsed:        %8lu kB\n"
				"ZeroPage:       %8lu kB\n"
				"NotifyFree:     %8lu kB\n"
				"FailReads:      %8lu kB\n"
				"FailWrites:     %8lu kB\n"
				"NumReads:       %8lu kB\n"
				"NumWrites:      %8lu kB\n"
				"InvalidIO:      %8lu kB\n"
#ifdef CONFIG_ZSM
				"ZSM saved:      %8lu kB\n"
				"ZSM4k saved:    %8lu kB\n"
#endif
				"MaxUsedPages:   %8lu kB\n"

				,
				B2K(zram_devices->disksize),
				P2K(atomic64_read(&zram_devices->stats.pages_stored)),
				B2K(atomic64_read(&zram_devices->stats.compr_data_size)),
				P2K(zs_get_total_pages(zram_devices->meta->mem_pool)),
				P2K(atomic64_read(&zram_devices->stats.zero_pages)),
				P2K(atomic64_read(&zram_devices->stats.notify_free)),
				P2K(atomic64_read(&zram_devices->stats.failed_reads)),
				P2K(atomic64_read(&zram_devices->stats.failed_writes)),
				P2K(atomic64_read(&zram_devices->stats.num_reads)),
				P2K(atomic64_read(&zram_devices->stats.num_writes)),
				P2K(atomic64_read(&zram_devices->stats.invalid_io)),
#ifdef CONFIG_ZSM
				B2K(atomic64_read(&zram_devices->stats.zsm_saved)),
				B2K(atomic64_read(&zram_devices->stats.zsm_saved4k)),
#endif
				P2K(atomic_long_read(&zram_devices->stats.max_used_pages))

				);
#undef P2K
#undef B2K
		seq_printf(m, "Algorithm: [%s]\n", zram_devices->compressor);
	}

	return 0;
}

static int zraminfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, zraminfo_proc_show, NULL);
}

static const struct file_operations zraminfo_proc_fops = {
	.open		= zraminfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

=======
static void destroy_devices(unsigned int nr)
{
	struct zram *zram;
	unsigned int i;

	for (i = 0; i < nr; i++) {
		zram = &zram_devices[i];

		zram_reset_device(zram);

		blk_cleanup_queue(zram->disk->queue);
		del_gendisk(zram->disk);
		put_disk(zram->disk);
	}

	kfree(zram_devices);
	unregister_blkdev(zram_major, "zram");
	pr_info("Destroyed %u device(s)\n", nr);
}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
static int __init zram_init(void)
{
	int ret, dev_id;

	if (num_devices > max_num_devices) {
		pr_warn("Invalid value for num_devices: %u\n",
				num_devices);
<<<<<<< HEAD
		ret = -EINVAL;
		goto out;
=======
		return -EINVAL;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}

	zram_major = register_blkdev(0, "zram");
	if (zram_major <= 0) {
		pr_warn("Unable to get major number\n");
<<<<<<< HEAD
		ret = -EBUSY;
		goto out;
	}

	/* Allocate the device array and initialize each one */
	zram_devices = kcalloc(num_devices, sizeof(struct zram), GFP_KERNEL);
	if (!zram_devices) {
		ret = -ENOMEM;
		goto unregister;
=======
		return -EBUSY;
	}

	/* Allocate the device array and initialize each one */
	zram_devices = kzalloc(num_devices * sizeof(struct zram), GFP_KERNEL);
	if (!zram_devices) {
		unregister_blkdev(zram_major, "zram");
		return -ENOMEM;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}

	for (dev_id = 0; dev_id < num_devices; dev_id++) {
		ret = create_device(&zram_devices[dev_id], dev_id);
		if (ret)
<<<<<<< HEAD
			goto free_devices;
	}

#ifdef CONFIG_PROC_FS
	proc_create("zraminfo", 0, NULL, &zraminfo_proc_fops);
#endif

	pr_info("Created %u device(s) ...\n", num_devices);

	return 0;

free_devices:
	while (dev_id)
		destroy_device(&zram_devices[--dev_id]);
	kfree(zram_devices);
unregister:
	unregister_blkdev(zram_major, "zram");
out:
=======
			goto out_error;
	}

	pr_info("Created %u device(s)\n", num_devices);
	return 0;

out_error:
	destroy_devices(dev_id);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return ret;
}

static void __exit zram_exit(void)
{
<<<<<<< HEAD
	int i;
	struct zram *zram;

	for (i = 0; i < num_devices; i++) {
		zram = &zram_devices[i];

		destroy_device(zram);
		/*
		 * Shouldn't access zram->disk after destroy_device
		 * because destroy_device already released zram->disk.
		 */
		zram_reset_device(zram, false);
	}

	unregister_blkdev(zram_major, "zram");

	kfree(zram_devices);
	pr_debug("Cleanup done!\n");
=======
	destroy_devices(num_devices);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

module_init(zram_init);
module_exit(zram_exit);

module_param(num_devices, uint, 0);
MODULE_PARM_DESC(num_devices, "Number of zram devices");

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Nitin Gupta <ngupta@vflare.org>");
MODULE_DESCRIPTION("Compressed RAM Block Device");
