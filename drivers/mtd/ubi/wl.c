/*
 * Copyright (c) International Business Machines Corp., 2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Authors: Artem Bityutskiy (Битюцкий Артём), Thomas Gleixner
 */

/*
 * UBI wear-leveling sub-system.
 *
 * This sub-system is responsible for wear-leveling. It works in terms of
 * physical eraseblocks and erase counters and knows nothing about logical
 * eraseblocks, volumes, etc. From this sub-system's perspective all physical
 * eraseblocks are of two types - used and free. Used physical eraseblocks are
 * those that were "get" by the 'ubi_wl_get_peb()' function, and free physical
 * eraseblocks are those that were put by the 'ubi_wl_put_peb()' function.
 *
 * Physical eraseblocks returned by 'ubi_wl_get_peb()' have only erase counter
 * header. The rest of the physical eraseblock contains only %0xFF bytes.
 *
 * When physical eraseblocks are returned to the WL sub-system by means of the
 * 'ubi_wl_put_peb()' function, they are scheduled for erasure. The erasure is
 * done asynchronously in context of the per-UBI device background thread,
 * which is also managed by the WL sub-system.
 *
 * The wear-leveling is ensured by means of moving the contents of used
 * physical eraseblocks with low erase counter to free physical eraseblocks
 * with high erase counter.
 *
 * If the WL sub-system fails to erase a physical eraseblock, it marks it as
 * bad.
 *
 * This sub-system is also responsible for scrubbing. If a bit-flip is detected
 * in a physical eraseblock, it has to be moved. Technically this is the same
 * as moving it for wear-leveling reasons.
 *
 * As it was said, for the UBI sub-system all physical eraseblocks are either
 * "free" or "used". Free eraseblock are kept in the @wl->free RB-tree, while
 * used eraseblocks are kept in @wl->used, @wl->erroneous, or @wl->scrub
 * RB-trees, as well as (temporarily) in the @wl->pq queue.
 *
 * When the WL sub-system returns a physical eraseblock, the physical
 * eraseblock is protected from being moved for some "time". For this reason,
 * the physical eraseblock is not directly moved from the @wl->free tree to the
 * @wl->used tree. There is a protection queue in between where this
 * physical eraseblock is temporarily stored (@wl->pq).
 *
 * All this protection stuff is needed because:
 *  o we don't want to move physical eraseblocks just after we have given them
 *    to the user; instead, we first want to let users fill them up with data;
 *
 *  o there is a chance that the user will put the physical eraseblock very
 *    soon, so it makes sense not to move it for some time, but wait.
 *
 * Physical eraseblocks stay protected only for limited time. But the "time" is
 * measured in erase cycles in this case. This is implemented with help of the
 * protection queue. Eraseblocks are put to the tail of this queue when they
 * are returned by the 'ubi_wl_get_peb()', and eraseblocks are removed from the
 * head of the queue on each erase operation (for any eraseblock). So the
 * length of the queue defines how may (global) erase cycles PEBs are protected.
 *
 * To put it differently, each physical eraseblock has 2 main states: free and
 * used. The former state corresponds to the @wl->free tree. The latter state
 * is split up on several sub-states:
 * o the WL movement is allowed (@wl->used tree);
 * o the WL movement is disallowed (@wl->erroneous) because the PEB is
 *   erroneous - e.g., there was a read error;
 * o the WL movement is temporarily prohibited (@wl->pq queue);
 * o scrubbing is needed (@wl->scrub tree).
 *
 * Depending on the sub-state, wear-leveling entries of the used physical
 * eraseblocks may be kept in one of those structures.
 *
 * Note, in this implementation, we keep a small in-RAM object for each physical
 * eraseblock. This is surely not a scalable solution. But it appears to be good
 * enough for moderately large flashes and it is simple. In future, one may
 * re-work this sub-system and make it more scalable.
 *
 * At the moment this sub-system does not utilize the sequence number, which
 * was introduced relatively recently. But it would be wise to do this because
 * the sequence number of a logical eraseblock characterizes how old is it. For
 * example, when we move a PEB with low erase counter, and we need to pick the
 * target PEB, we pick a PEB with the highest EC if our PEB is "old" and we
 * pick target PEB with an average EC if our PEB is not very "old". This is a
 * room for future re-works of the WL sub-system.
 */

#include <linux/slab.h>
#include <linux/crc32.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include "ubi.h"

/* Number of physical eraseblocks reserved for wear-leveling purposes */
#define WL_RESERVED_PEBS 1

/*
 * Maximum difference between two erase counters. If this threshold is
 * exceeded, the WL sub-system starts moving data from used physical
 * eraseblocks with low erase counter to free physical eraseblocks with high
 * erase counter.
 */
#define UBI_WL_THRESHOLD CONFIG_MTD_UBI_WL_THRESHOLD

/*
 * When a physical eraseblock is moved, the WL sub-system has to pick the target
 * physical eraseblock to move to. The simplest way would be just to pick the
 * one with the highest erase counter. But in certain workloads this could lead
 * to an unlimited wear of one or few physical eraseblock. Indeed, imagine a
 * situation when the picked physical eraseblock is constantly erased after the
 * data is written to it. So, we have a constant which limits the highest erase
 * counter of the free physical eraseblock to pick. Namely, the WL sub-system
 * does not pick eraseblocks with erase counter greater than the lowest erase
 * counter plus %WL_FREE_MAX_DIFF.
 */
#define WL_FREE_MAX_DIFF (2*UBI_WL_THRESHOLD)

/*
 * Maximum number of consecutive background thread failures which is enough to
 * switch to read-only mode.
 */
#define WL_MAX_FAILURES 32

static int self_check_ec(struct ubi_device *ubi, int pnum, int ec);
static int self_check_in_wl_tree(const struct ubi_device *ubi,
				 struct ubi_wl_entry *e, struct rb_root *root);
static int self_check_in_pq(const struct ubi_device *ubi,
			    struct ubi_wl_entry *e);

#ifdef CONFIG_MTD_UBI_FASTMAP
/**
 * update_fastmap_work_fn - calls ubi_update_fastmap from a work queue
 * @wrk: the work description object
 */
static void update_fastmap_work_fn(struct work_struct *wrk)
{
	struct ubi_device *ubi = container_of(wrk, struct ubi_device, fm_work);
<<<<<<< HEAD

	ubi_update_fastmap(ubi);
=======
	ubi_update_fastmap(ubi);
	spin_lock(&ubi->wl_lock);
	ubi->fm_work_scheduled = 0;
	spin_unlock(&ubi->wl_lock);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

/**
 *  ubi_ubi_is_fm_block - returns 1 if a PEB is currently used in a fastmap.
 *  @ubi: UBI device description object
 *  @pnum: the to be checked PEB
 */
static int ubi_is_fm_block(struct ubi_device *ubi, int pnum)
{
	int i;

	if (!ubi->fm)
		return 0;

	for (i = 0; i < ubi->fm->used_blocks; i++)
		if (ubi->fm->e[i]->pnum == pnum)
			return 1;

	return 0;
}
#else
static int ubi_is_fm_block(struct ubi_device *ubi, int pnum)
{
	return 0;
}
#endif

/**
 * wl_tree_add - add a wear-leveling entry to a WL RB-tree.
 * @e: the wear-leveling entry to add
 * @root: the root of the tree
 *
 * Note, we use (erase counter, physical eraseblock number) pairs as keys in
 * the @ubi->used and @ubi->free RB-trees.
 */
static void wl_tree_add(struct ubi_wl_entry *e, struct rb_root *root)
{
	struct rb_node **p, *parent = NULL;

	p = &root->rb_node;
	while (*p) {
		struct ubi_wl_entry *e1;

		parent = *p;
		e1 = rb_entry(parent, struct ubi_wl_entry, u.rb);

		if (e->ec < e1->ec)
			p = &(*p)->rb_left;
		else if (e->ec > e1->ec)
			p = &(*p)->rb_right;
		else {
			ubi_assert(e->pnum != e1->pnum);
			if (e->pnum < e1->pnum)
				p = &(*p)->rb_left;
			else
				p = &(*p)->rb_right;
		}
	}

	rb_link_node(&e->u.rb, parent, p);
	rb_insert_color(&e->u.rb, root);
}

/**
 * do_work - do one pending work.
 * @ubi: UBI device description object
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 */
static int do_work(struct ubi_device *ubi)
{
	int err;
	struct ubi_work *wrk;

	cond_resched();

	/*
	 * @ubi->work_sem is used to synchronize with the workers. Workers take
	 * it in read mode, so many of them may be doing works at a time. But
	 * the queue flush code has to be sure the whole queue of works is
	 * done, and it takes the mutex in write mode.
	 */
	down_read(&ubi->work_sem);
	spin_lock(&ubi->wl_lock);
	if (list_empty(&ubi->works)) {
		spin_unlock(&ubi->wl_lock);
		up_read(&ubi->work_sem);
		return 0;
	}

	wrk = list_entry(ubi->works.next, struct ubi_work, list);
	list_del(&wrk->list);
	ubi->works_count -= 1;
	ubi_assert(ubi->works_count >= 0);
	spin_unlock(&ubi->wl_lock);

	/*
	 * Call the worker function. Do not touch the work structure
	 * after this call as it will have been freed or reused by that
	 * time by the worker function.
	 */
	err = wrk->func(ubi, wrk, 0);
	if (err)
		ubi_err("work failed with error code %d", err);
	up_read(&ubi->work_sem);

	return err;
}

/**
 * produce_free_peb - produce a free physical eraseblock.
 * @ubi: UBI device description object
 *
 * This function tries to make a free PEB by means of synchronous execution of
 * pending works. This may be needed if, for example the background thread is
 * disabled. Returns zero in case of success and a negative error code in case
 * of failure.
 */
static int produce_free_peb(struct ubi_device *ubi)
{
	int err;

	while (!ubi->free.rb_node && ubi->works_count) {
		spin_unlock(&ubi->wl_lock);

		dbg_wl("do one work synchronously");
		err = do_work(ubi);

		spin_lock(&ubi->wl_lock);
		if (err)
			return err;
	}

	return 0;
}

/**
 * in_wl_tree - check if wear-leveling entry is present in a WL RB-tree.
 * @e: the wear-leveling entry to check
 * @root: the root of the tree
 *
 * This function returns non-zero if @e is in the @root RB-tree and zero if it
 * is not.
 */
static int in_wl_tree(struct ubi_wl_entry *e, struct rb_root *root)
{
	struct rb_node *p;

	p = root->rb_node;
	while (p) {
		struct ubi_wl_entry *e1;

		e1 = rb_entry(p, struct ubi_wl_entry, u.rb);

		if (e->pnum == e1->pnum) {
			ubi_assert(e == e1);
			return 1;
		}

		if (e->ec < e1->ec)
			p = p->rb_left;
		else if (e->ec > e1->ec)
			p = p->rb_right;
		else {
			ubi_assert(e->pnum != e1->pnum);
			if (e->pnum < e1->pnum)
				p = p->rb_left;
			else
				p = p->rb_right;
		}
	}

	return 0;
}

/**
 * prot_queue_add - add physical eraseblock to the protection queue.
 * @ubi: UBI device description object
 * @e: the physical eraseblock to add
 *
 * This function adds @e to the tail of the protection queue @ubi->pq, where
 * @e will stay for %UBI_PROT_QUEUE_LEN erase operations and will be
 * temporarily protected from the wear-leveling worker. Note, @wl->lock has to
 * be locked.
 */
static void prot_queue_add(struct ubi_device *ubi, struct ubi_wl_entry *e)
{
	int pq_tail = ubi->pq_head - 1;

	if (pq_tail < 0)
		pq_tail = UBI_PROT_QUEUE_LEN - 1;
	ubi_assert(pq_tail >= 0 && pq_tail < UBI_PROT_QUEUE_LEN);
	list_add_tail(&e->u.list, &ubi->pq[pq_tail]);
	dbg_wl("added PEB %d EC %d to the protection queue", e->pnum, e->ec);
}

/**
 * find_wl_entry - find wear-leveling entry closest to certain erase counter.
 * @ubi: UBI device description object
 * @root: the RB-tree where to look for
 * @diff: maximum possible difference from the smallest erase counter
 *
 * This function looks for a wear leveling entry with erase counter closest to
 * min + @diff, where min is the smallest erase counter.
 */
static struct ubi_wl_entry *find_wl_entry(struct ubi_device *ubi,
					  struct rb_root *root, int diff)
{
	struct rb_node *p;
	struct ubi_wl_entry *e, *prev_e = NULL;
	int max;

	e = rb_entry(rb_first(root), struct ubi_wl_entry, u.rb);
	max = e->ec + diff;

	p = root->rb_node;
	while (p) {
		struct ubi_wl_entry *e1;

		e1 = rb_entry(p, struct ubi_wl_entry, u.rb);
		if (e1->ec >= max)
			p = p->rb_left;
		else {
			p = p->rb_right;
			prev_e = e;
			e = e1;
		}
	}

	/* If no fastmap has been written and this WL entry can be used
	 * as anchor PEB, hold it back and return the second best WL entry
	 * such that fastmap can use the anchor PEB later. */
	if (prev_e && !ubi->fm_disabled &&
	    !ubi->fm && e->pnum < UBI_FM_MAX_START)
		return prev_e;

	return e;
}

/**
 * find_mean_wl_entry - find wear-leveling entry with medium erase counter.
 * @ubi: UBI device description object
 * @root: the RB-tree where to look for
 *
 * This function looks for a wear leveling entry with medium erase counter,
 * but not greater or equivalent than the lowest erase counter plus
 * %WL_FREE_MAX_DIFF/2.
 */
static struct ubi_wl_entry *find_mean_wl_entry(struct ubi_device *ubi,
					       struct rb_root *root)
{
	struct ubi_wl_entry *e, *first, *last;

	first = rb_entry(rb_first(root), struct ubi_wl_entry, u.rb);
	last = rb_entry(rb_last(root), struct ubi_wl_entry, u.rb);

<<<<<<< HEAD
	if (last->ec - first->ec < ubi->wl_th*2) {
=======
	if (last->ec - first->ec < WL_FREE_MAX_DIFF) {
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		e = rb_entry(root->rb_node, struct ubi_wl_entry, u.rb);

#ifdef CONFIG_MTD_UBI_FASTMAP
		/* If no fastmap has been written and this WL entry can be used
		 * as anchor PEB, hold it back and return the second best
		 * WL entry such that fastmap can use the anchor PEB later. */
		if (e && !ubi->fm_disabled && !ubi->fm &&
		    e->pnum < UBI_FM_MAX_START)
			e = rb_entry(rb_next(root->rb_node),
				     struct ubi_wl_entry, u.rb);
#endif
	} else
<<<<<<< HEAD
		e = find_wl_entry(ubi, root, ubi->wl_th);
=======
		e = find_wl_entry(ubi, root, WL_FREE_MAX_DIFF/2);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	return e;
}

#ifdef CONFIG_MTD_UBI_FASTMAP
/**
 * find_anchor_wl_entry - find wear-leveling entry to used as anchor PEB.
 * @root: the RB-tree where to look for
 */
static struct ubi_wl_entry *find_anchor_wl_entry(struct rb_root *root)
{
	struct rb_node *p;
	struct ubi_wl_entry *e, *victim = NULL;
	int max_ec = UBI_MAX_ERASECOUNTER;

	ubi_rb_for_each_entry(p, e, root, u.rb) {
		if (e->pnum < UBI_FM_MAX_START && e->ec < max_ec) {
			victim = e;
			max_ec = e->ec;
		}
	}

	return victim;
}

static int anchor_pebs_avalible(struct rb_root *root)
{
	struct rb_node *p;
	struct ubi_wl_entry *e;

	ubi_rb_for_each_entry(p, e, root, u.rb)
		if (e->pnum < UBI_FM_MAX_START)
			return 1;

	return 0;
}

/**
 * ubi_wl_get_fm_peb - find a physical erase block with a given maximal number.
 * @ubi: UBI device description object
 * @anchor: This PEB will be used as anchor PEB by fastmap
 *
 * The function returns a physical erase block with a given maximal number
 * and removes it from the wl subsystem.
 * Must be called with wl_lock held!
 */
struct ubi_wl_entry *ubi_wl_get_fm_peb(struct ubi_device *ubi, int anchor)
{
	struct ubi_wl_entry *e = NULL;

	if (!ubi->free.rb_node || (ubi->free_count - ubi->beb_rsvd_pebs < 1))
		goto out;

	if (anchor)
		e = find_anchor_wl_entry(&ubi->free);
	else
		e = find_mean_wl_entry(ubi, &ubi->free);

	if (!e)
		goto out;

	self_check_in_wl_tree(ubi, e, &ubi->free);

	/* remove it from the free list,
	 * the wl subsystem does no longer know this erase block */
	rb_erase(&e->u.rb, &ubi->free);
	ubi->free_count--;
out:
	return e;
}
#endif

/**
 * __wl_get_peb - get a physical eraseblock.
 * @ubi: UBI device description object
 *
 * This function returns a physical eraseblock in case of success and a
 * negative error code in case of failure.
 */
static int __wl_get_peb(struct ubi_device *ubi)
{
	int err;
	struct ubi_wl_entry *e;

retry:
	if (!ubi->free.rb_node) {
		if (ubi->works_count == 0) {
			ubi_err("no free eraseblocks");
			ubi_assert(list_empty(&ubi->works));
			return -ENOSPC;
		}

		err = produce_free_peb(ubi);
		if (err < 0)
			return err;
		goto retry;
	}

	e = find_mean_wl_entry(ubi, &ubi->free);
	if (!e) {
		ubi_err("no free eraseblocks");
		return -ENOSPC;
	}

	self_check_in_wl_tree(ubi, e, &ubi->free);

	/*
	 * Move the physical eraseblock to the protection queue where it will
	 * be protected from being moved for some time.
	 */
	rb_erase(&e->u.rb, &ubi->free);
	ubi->free_count--;
	dbg_wl("PEB %d EC %d", e->pnum, e->ec);
#ifndef CONFIG_MTD_UBI_FASTMAP
	/* We have to enqueue e only if fastmap is disabled,
	 * is fastmap enabled prot_queue_add() will be called by
	 * ubi_wl_get_peb() after removing e from the pool. */
	prot_queue_add(ubi, e);
#endif
	return e->pnum;
}

#ifdef CONFIG_MTD_UBI_FASTMAP
/**
 * return_unused_pool_pebs - returns unused PEB to the free tree.
 * @ubi: UBI device description object
 * @pool: fastmap pool description object
 */
static void return_unused_pool_pebs(struct ubi_device *ubi,
				    struct ubi_fm_pool *pool)
{
	int i;
	struct ubi_wl_entry *e;

	for (i = pool->used; i < pool->size; i++) {
		e = ubi->lookuptbl[pool->pebs[i]];
		wl_tree_add(e, &ubi->free);
		ubi->free_count++;
	}
}

/**
 * refill_wl_pool - refills all the fastmap pool used by the
 * WL sub-system.
 * @ubi: UBI device description object
 */
static void refill_wl_pool(struct ubi_device *ubi)
{
	struct ubi_wl_entry *e;
	struct ubi_fm_pool *pool = &ubi->fm_wl_pool;

	return_unused_pool_pebs(ubi, pool);

	for (pool->size = 0; pool->size < pool->max_size; pool->size++) {
		if (!ubi->free.rb_node ||
		   (ubi->free_count - ubi->beb_rsvd_pebs < 5))
			break;

<<<<<<< HEAD
		e = find_wl_entry(ubi, &ubi->free, ubi->wl_th*2);
=======
		e = find_wl_entry(ubi, &ubi->free, WL_FREE_MAX_DIFF);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		self_check_in_wl_tree(ubi, e, &ubi->free);
		rb_erase(&e->u.rb, &ubi->free);
		ubi->free_count--;

		pool->pebs[pool->size] = e->pnum;
	}
	pool->used = 0;
}

/**
 * refill_wl_user_pool - refills all the fastmap pool used by ubi_wl_get_peb.
 * @ubi: UBI device description object
 */
static void refill_wl_user_pool(struct ubi_device *ubi)
{
	struct ubi_fm_pool *pool = &ubi->fm_pool;

	return_unused_pool_pebs(ubi, pool);

	for (pool->size = 0; pool->size < pool->max_size; pool->size++) {
		pool->pebs[pool->size] = __wl_get_peb(ubi);
		if (pool->pebs[pool->size] < 0)
			break;
	}
	pool->used = 0;
}

/**
 * ubi_refill_pools - refills all fastmap PEB pools.
 * @ubi: UBI device description object
 */
void ubi_refill_pools(struct ubi_device *ubi)
{
	spin_lock(&ubi->wl_lock);
	refill_wl_pool(ubi);
	refill_wl_user_pool(ubi);
	spin_unlock(&ubi->wl_lock);
}

/* ubi_wl_get_peb - works exaclty like __wl_get_peb but keeps track of
 * the fastmap pool.
 */
int ubi_wl_get_peb(struct ubi_device *ubi)
{
	int ret;
	struct ubi_fm_pool *pool = &ubi->fm_pool;
	struct ubi_fm_pool *wl_pool = &ubi->fm_wl_pool;

	if (!pool->size || !wl_pool->size || pool->used == pool->size ||
	    wl_pool->used == wl_pool->size)
		ubi_update_fastmap(ubi);

	/* we got not a single free PEB */
	if (!pool->size)
		ret = -ENOSPC;
	else {
		spin_lock(&ubi->wl_lock);
		ret = pool->pebs[pool->used++];
		prot_queue_add(ubi, ubi->lookuptbl[ret]);
		spin_unlock(&ubi->wl_lock);
	}

	return ret;
}

/* get_peb_for_wl - returns a PEB to be used internally by the WL sub-system.
 *
 * @ubi: UBI device description object
 */
static struct ubi_wl_entry *get_peb_for_wl(struct ubi_device *ubi)
{
	struct ubi_fm_pool *pool = &ubi->fm_wl_pool;
	int pnum;

	if (pool->used == pool->size || !pool->size) {
		/* We cannot update the fastmap here because this
		 * function is called in atomic context.
		 * Let's fail here and refill/update it as soon as possible. */
<<<<<<< HEAD
		schedule_work(&ubi->fm_work);
		return NULL;
	}
	pnum = pool->pebs[pool->used++];
	return ubi->lookuptbl[pnum];
}
#else
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
struct ubi_wl_entry *get_peb_for_tlc_wl(struct ubi_device *ubi)
{
	struct ubi_wl_entry *e;

	e = find_wl_entry(ubi, &ubi->tlc_free, ubi->tlc_wl_th * 2);
	self_check_in_wl_tree(ubi, e, &ubi->tlc_free);
	rb_erase(&e->u.rb, &ubi->tlc_free);
	ubi->tlc_free_count--;
	/*pr_err("\n[GET TLC] free count %d\n", ubi->tlc_free_count);*/
	return e;
}
#endif
=======
		if (!ubi->fm_work_scheduled) {
			ubi->fm_work_scheduled = 1;
			schedule_work(&ubi->fm_work);
		}
		return NULL;
	} else {
		pnum = pool->pebs[pool->used++];
		return ubi->lookuptbl[pnum];
	}
}
#else
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
static struct ubi_wl_entry *get_peb_for_wl(struct ubi_device *ubi)
{
	struct ubi_wl_entry *e;

<<<<<<< HEAD
	e = find_wl_entry(ubi, &ubi->free, ubi->wl_th*2);
=======
	e = find_wl_entry(ubi, &ubi->free, WL_FREE_MAX_DIFF);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	self_check_in_wl_tree(ubi, e, &ubi->free);
	ubi->free_count--;
	ubi_assert(ubi->free_count >= 0);
	rb_erase(&e->u.rb, &ubi->free);

	return e;
}
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
struct ubi_wl_entry *ubi_wl_get_tlc_peb(struct ubi_device *ubi)
{
	struct ubi_wl_entry *e = get_peb_for_tlc_wl(ubi);

	prot_queue_add(ubi, e);
	return e;
}
#endif
=======

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
int ubi_wl_get_peb(struct ubi_device *ubi)
{
	int peb, err;

	spin_lock(&ubi->wl_lock);
	peb = __wl_get_peb(ubi);
	spin_unlock(&ubi->wl_lock);

	if (peb < 0)
		return peb;

	err = ubi_self_check_all_ff(ubi, peb, ubi->vid_hdr_aloffset,
				    ubi->peb_size - ubi->vid_hdr_aloffset);
	if (err) {
		ubi_err("new PEB %d does not contain all 0xFF bytes", peb);
		return err;
	}

	return peb;
}
#endif

/**
 * prot_queue_del - remove a physical eraseblock from the protection queue.
 * @ubi: UBI device description object
 * @pnum: the physical eraseblock to remove
 *
 * This function deletes PEB @pnum from the protection queue and returns zero
 * in case of success and %-ENODEV if the PEB was not found.
 */
static int prot_queue_del(struct ubi_device *ubi, int pnum)
{
	struct ubi_wl_entry *e;

	e = ubi->lookuptbl[pnum];
	if (!e)
		return -ENODEV;

	if (self_check_in_pq(ubi, e))
		return -ENODEV;

	list_del(&e->u.list);
	dbg_wl("deleted PEB %d from the protection queue", e->pnum);
	return 0;
}

/**
 * sync_erase - synchronously erase a physical eraseblock.
 * @ubi: UBI device description object
 * @e: the the physical eraseblock to erase
 * @torture: if the physical eraseblock has to be tortured
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 */
<<<<<<< HEAD
int sync_erase(struct ubi_device *ubi, struct ubi_wl_entry *e,
=======
static int sync_erase(struct ubi_device *ubi, struct ubi_wl_entry *e,
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		      int torture)
{
	int err;
	struct ubi_ec_hdr *ec_hdr;
<<<<<<< HEAD
	unsigned long long old_ec = e->ec, ec = e->ec; /*MTK: old_ec*/
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	if (e->tlc) {
		ubi_msg("[Bean]erase TLC PEB %d, old EC %llu, free count %d return directly\n",
			e->pnum, ec, ubi->tlc_free_count);
		err = ubi_change_mtbl_record(ubi, e->pnum, 0, 0, 0);
		return err;
	}
#endif
=======
	unsigned long long ec = e->ec;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	dbg_wl("erase PEB %d, old EC %llu", e->pnum, ec);

	err = self_check_ec(ubi, e->pnum, e->ec);
	if (err)
		return -EINVAL;

<<<<<<< HEAD
	ec_hdr = vzalloc(ubi->ec_hdr_alsize);
=======
	ec_hdr = kzalloc(ubi->ec_hdr_alsize, GFP_NOFS);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (!ec_hdr)
		return -ENOMEM;

	err = ubi_io_sync_erase(ubi, e->pnum, torture);
	if (err < 0)
		goto out_free;

	ec += err;
	if (ec > UBI_MAX_ERASECOUNTER) {
		/*
		 * Erase counter overflow. Upgrade UBI and use 64-bit
		 * erase counters internally.
		 */
		ubi_err("erase counter overflow at PEB %d, EC %llu",
			e->pnum, ec);
		err = -EINVAL;
		goto out_free;
	}

	dbg_wl("erased PEB %d, new EC %llu", e->pnum, ec);

	ec_hdr->ec = cpu_to_be64(ec);

	err = ubi_io_write_ec_hdr(ubi, e->pnum, ec_hdr);
	if (err)
		goto out_free;

	e->ec = ec;
	spin_lock(&ubi->wl_lock);
	if (e->ec > ubi->max_ec)
		ubi->max_ec = e->ec;
<<<<<<< HEAD
/*MTK start: the incresing of ec > 1 is doing by torture*/
	if (ec - old_ec > 1)
		ubi->torture += (ec - old_ec);
	ubi->ec_sum += (ec - old_ec);
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	ubi->mean_ec = div_u64(ubi->ec_sum, (ubi->peb_count - ubi->mtbl_slots));
#else
	ubi->mean_ec = div_u64(ubi->ec_sum, ubi->rsvd_pebs);
#endif
/*MTK end*/
	spin_unlock(&ubi->wl_lock);

out_free:
	vfree(ec_hdr);
=======
	spin_unlock(&ubi->wl_lock);

out_free:
	kfree(ec_hdr);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return err;
}

/**
 * serve_prot_queue - check if it is time to stop protecting PEBs.
 * @ubi: UBI device description object
 *
 * This function is called after each erase operation and removes PEBs from the
 * tail of the protection queue. These PEBs have been protected for long enough
 * and should be moved to the used tree.
 */
static void serve_prot_queue(struct ubi_device *ubi)
{
	struct ubi_wl_entry *e, *tmp;
	int count;

	/*
	 * There may be several protected physical eraseblock to remove,
	 * process them all.
	 */
repeat:
	count = 0;
	spin_lock(&ubi->wl_lock);
	list_for_each_entry_safe(e, tmp, &ubi->pq[ubi->pq_head], u.list) {
		dbg_wl("PEB %d EC %d protection over, move to used tree",
			e->pnum, e->ec);

		list_del(&e->u.list);
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		if (e->tlc)
			wl_tree_add(e, &ubi->tlc_used);
		else
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		wl_tree_add(e, &ubi->used);
		if (count++ > 32) {
			/*
			 * Let's be nice and avoid holding the spinlock for
			 * too long.
			 */
			spin_unlock(&ubi->wl_lock);
			cond_resched();
			goto repeat;
		}
	}

	ubi->pq_head += 1;
	if (ubi->pq_head == UBI_PROT_QUEUE_LEN)
		ubi->pq_head = 0;
	ubi_assert(ubi->pq_head >= 0 && ubi->pq_head < UBI_PROT_QUEUE_LEN);
	spin_unlock(&ubi->wl_lock);
}

/**
 * __schedule_ubi_work - schedule a work.
 * @ubi: UBI device description object
 * @wrk: the work to schedule
 *
 * This function adds a work defined by @wrk to the tail of the pending works
 * list. Can only be used if ubi->work_sem is already held in read mode!
 */
static void __schedule_ubi_work(struct ubi_device *ubi, struct ubi_work *wrk)
{
	spin_lock(&ubi->wl_lock);
	list_add_tail(&wrk->list, &ubi->works);
	ubi_assert(ubi->works_count >= 0);
	ubi->works_count += 1;
	if (ubi->thread_enabled && !ubi_dbg_is_bgt_disabled(ubi))
		wake_up_process(ubi->bgt_thread);
	spin_unlock(&ubi->wl_lock);
}

/**
 * schedule_ubi_work - schedule a work.
 * @ubi: UBI device description object
 * @wrk: the work to schedule
 *
 * This function adds a work defined by @wrk to the tail of the pending works
 * list.
 */
static void schedule_ubi_work(struct ubi_device *ubi, struct ubi_work *wrk)
{
	down_read(&ubi->work_sem);
	__schedule_ubi_work(ubi, wrk);
	up_read(&ubi->work_sem);
}

static int erase_worker(struct ubi_device *ubi, struct ubi_work *wl_wrk,
			int shutdown);

#ifdef CONFIG_MTD_UBI_FASTMAP
/**
 * ubi_is_erase_work - checks whether a work is erase work.
 * @wrk: The work object to be checked
 */
int ubi_is_erase_work(struct ubi_work *wrk)
{
	return wrk->func == erase_worker;
}
#endif

/**
 * schedule_erase - schedule an erase work.
 * @ubi: UBI device description object
 * @e: the WL entry of the physical eraseblock to erase
 * @vol_id: the volume ID that last used this PEB
 * @lnum: the last used logical eraseblock number for the PEB
 * @torture: if the physical eraseblock has to be tortured
 *
 * This function returns zero in case of success and a %-ENOMEM in case of
 * failure.
 */
static int schedule_erase(struct ubi_device *ubi, struct ubi_wl_entry *e,
			  int vol_id, int lnum, int torture)
{
	struct ubi_work *wl_wrk;

	ubi_assert(e);
	ubi_assert(!ubi_is_fm_block(ubi, e->pnum));

	dbg_wl("schedule erasure of PEB %d, EC %d, torture %d",
	       e->pnum, e->ec, torture);

	wl_wrk = kmalloc(sizeof(struct ubi_work), GFP_NOFS);
	if (!wl_wrk)
		return -ENOMEM;

	wl_wrk->func = &erase_worker;
	wl_wrk->e = e;
	wl_wrk->vol_id = vol_id;
	wl_wrk->lnum = lnum;
	wl_wrk->torture = torture;

	schedule_ubi_work(ubi, wl_wrk);
	return 0;
}

/**
 * do_sync_erase - run the erase worker synchronously.
 * @ubi: UBI device description object
 * @e: the WL entry of the physical eraseblock to erase
 * @vol_id: the volume ID that last used this PEB
 * @lnum: the last used logical eraseblock number for the PEB
 * @torture: if the physical eraseblock has to be tortured
 *
 */
static int do_sync_erase(struct ubi_device *ubi, struct ubi_wl_entry *e,
			 int vol_id, int lnum, int torture)
{
	struct ubi_work *wl_wrk;

	dbg_wl("sync erase of PEB %i", e->pnum);

	wl_wrk = kmalloc(sizeof(struct ubi_work), GFP_NOFS);
	if (!wl_wrk)
		return -ENOMEM;

	wl_wrk->e = e;
	wl_wrk->vol_id = vol_id;
	wl_wrk->lnum = lnum;
	wl_wrk->torture = torture;

	return erase_worker(ubi, wl_wrk, 0);
}

#ifdef CONFIG_MTD_UBI_FASTMAP
/**
 * ubi_wl_put_fm_peb - returns a PEB used in a fastmap to the wear-leveling
 * sub-system.
 * see: ubi_wl_put_peb()
 *
 * @ubi: UBI device description object
 * @fm_e: physical eraseblock to return
 * @lnum: the last used logical eraseblock number for the PEB
 * @torture: if this physical eraseblock has to be tortured
 */
int ubi_wl_put_fm_peb(struct ubi_device *ubi, struct ubi_wl_entry *fm_e,
		      int lnum, int torture)
{
	struct ubi_wl_entry *e;
	int vol_id, pnum = fm_e->pnum;

	dbg_wl("PEB %d", pnum);

	ubi_assert(pnum >= 0);
	ubi_assert(pnum < ubi->peb_count);

	spin_lock(&ubi->wl_lock);
	e = ubi->lookuptbl[pnum];

	/* This can happen if we recovered from a fastmap the very
	 * first time and writing now a new one. In this case the wl system
	 * has never seen any PEB used by the original fastmap.
	 */
	if (!e) {
		e = fm_e;
		ubi_assert(e->ec >= 0);
		ubi->lookuptbl[pnum] = e;
	} else {
		e->ec = fm_e->ec;
		kfree(fm_e);
	}

	spin_unlock(&ubi->wl_lock);

	vol_id = lnum ? UBI_FM_DATA_VOLUME_ID : UBI_FM_SB_VOLUME_ID;
	return schedule_erase(ubi, e, vol_id, lnum, torture);
}
#endif

/**
 * wear_leveling_worker - wear-leveling worker function.
 * @ubi: UBI device description object
 * @wrk: the work object
 * @shutdown: non-zero if the worker has to free memory and exit
 * because the WL-subsystem is shutting down
 *
 * This function copies a more worn out physical eraseblock to a less worn out
 * one. Returns zero in case of success and a negative error code in case of
 * failure.
 */
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
static int wear_leveling_worker(struct ubi_device *ubi, struct ubi_work *wrk,
				int shutdown)
{
	int err, archiving = 0, scrubbing = 0, torture = 0, protect = 0, erroneous = 0, erase_e2 = 1;
	int vol_id = -1, lnum = -1;
	struct ubi_wl_entry *e1, *e2;
	struct ubi_vid_hdr *vid_hdr;
	int do_wl = 0; /*MTK:wl or not, 1 for wl, 2 for scrubbing, 3 for archiving */
=======
static int wear_leveling_worker(struct ubi_device *ubi, struct ubi_work *wrk,
				int shutdown)
{
	int err, scrubbing = 0, torture = 0, protect = 0, erroneous = 0;
	int erase = 0, keep = 0, vol_id = -1, lnum = -1;
#ifdef CONFIG_MTD_UBI_FASTMAP
	int anchor = wrk->anchor;
#endif
	struct ubi_wl_entry *e1, *e2;
	struct ubi_vid_hdr *vid_hdr;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	kfree(wrk);
	if (shutdown)
		return 0;

	vid_hdr = ubi_zalloc_vid_hdr(ubi, GFP_NOFS);
	if (!vid_hdr)
		return -ENOMEM;

<<<<<<< HEAD
	/*ubi_err("1");*/
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	mutex_lock(&ubi->move_mutex);
	spin_lock(&ubi->wl_lock);
	ubi_assert(!ubi->move_from && !ubi->move_to);
	ubi_assert(!ubi->move_to_put);

<<<<<<< HEAD
	if (!ubi->tlc_used.rb_node && !ubi->scrub.rb_node && !ubi->archive.rb_node) {
		ubi_err("cancel WL, a list is empty: tlc_free %d, tlc_used %d srcub %d, archive %d",
		       !ubi->tlc_free.rb_node, !ubi->tlc_used.rb_node, !ubi->scrub.rb_node, !ubi->archive.rb_node);
		goto out_cancel;
	}


	/*ubi_err("2");*/
	if (ubi->scrub.rb_node) {
		/*ubi_err("[Bean Scrub]");*/
		/* Perform scrubbing */
		scrubbing = 1;
		e1 = rb_entry(rb_first(&ubi->scrub), struct ubi_wl_entry, u.rb);
		if (e1->tlc) {
			if (ubi->tlc_free.rb_node == NULL) {
				ubi_err("no free tlc peb cancel scrub PEB %d", e1->pnum);
				goto out_cancel;
			}
			e2 = get_peb_for_tlc_wl(ubi);
		} else {
			if (ubi->free.rb_node == NULL) {
				ubi_err("no free peb cancel scrub PEB %d", e1->pnum);
				goto out_cancel;
			}
			e2 = get_peb_for_wl(ubi);
		}

		self_check_in_wl_tree(ubi, e1, &ubi->scrub);
		rb_erase(&e1->u.rb, &ubi->scrub);
		ubi_msg("scrub move PEB %d EC %d to PEB %d EC %d", e1->pnum, e1->ec, e2->pnum, e2->ec);
		do_wl = 2;                /*MTK*/
	} else {
		e1 = rb_entry(rb_first(&ubi->tlc_used), struct ubi_wl_entry, u.rb);
		if (ubi->tlc_free.rb_node == NULL) {
			ubi_err("no free tlc peb cancel wl PEB %d", e1->pnum);
			goto out_cancel;
		}
		e2 = get_peb_for_tlc_wl(ubi);
		if ((e1 == NULL || e2->ec - e1->ec < ubi->tlc_wl_th) && ubi->tlc_free_count > 0) {
			archiving = 1;
			e1 = rb_entry(rb_first(&ubi->archive), struct ubi_wl_entry, u.rb);
			/*ubi_assert(e1);*/
			if (e1 == NULL) {
				if (e2->tlc) { /*get_peb_for_tlc_wl(ubi);*/
					wl_tree_add(e2, &ubi->tlc_free);
					ubi->tlc_free_count++;
					goto out_cancel;
				}
				ubi_assert(e1);
			}
			self_check_in_wl_tree(ubi, e1, &ubi->archive);
			rb_erase(&e1->u.rb, &ubi->archive);
			/*pr_err("[Bean]archive PEB %d EC %d to PEB %d EC %d tlc_free_count %d\n",
						e1->pnum, e1->ec, e2->pnum, e2->ec, ubi->tlc_free_count);*/
			ubi_msg("archive move PEB %d EC %d to PEB %d EC %d tlc_free_count %d",
						e1->pnum, e1->ec, e2->pnum, e2->ec, ubi->tlc_free_count);
			do_wl = 3;
		} else if (e2->ec - e1->ec >= ubi->tlc_wl_th) {
			/*ubi_err("[Bean WL]");*/
			self_check_in_wl_tree(ubi, e1, &ubi->tlc_used);
			rb_erase(&e1->u.rb, &ubi->tlc_used);
			ubi_msg("WL move PEB %d EC %d to PEB %d EC %d", e1->pnum, e1->ec, e2->pnum, e2->ec);
			do_wl = 1;
		} else {
			ubi_msg("no WL needed: min used EC %d, max free EC %d",
					!e1 ? -1 : e1->ec, e2->ec);
			/* Give the unused PEB back */
			wl_tree_add(e2, &ubi->tlc_free);
			ubi->tlc_free_count++;
			goto out_cancel;
		}
	}
	/*ubi_err("6");*/
=======
	if (!ubi->free.rb_node ||
	    (!ubi->used.rb_node && !ubi->scrub.rb_node)) {
		/*
		 * No free physical eraseblocks? Well, they must be waiting in
		 * the queue to be erased. Cancel movement - it will be
		 * triggered again when a free physical eraseblock appears.
		 *
		 * No used physical eraseblocks? They must be temporarily
		 * protected from being moved. They will be moved to the
		 * @ubi->used tree later and the wear-leveling will be
		 * triggered again.
		 */
		dbg_wl("cancel WL, a list is empty: free %d, used %d",
		       !ubi->free.rb_node, !ubi->used.rb_node);
		goto out_cancel;
	}

#ifdef CONFIG_MTD_UBI_FASTMAP
	/* Check whether we need to produce an anchor PEB */
	if (!anchor)
		anchor = !anchor_pebs_avalible(&ubi->free);

	if (anchor) {
		e1 = find_anchor_wl_entry(&ubi->used);
		if (!e1)
			goto out_cancel;
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		self_check_in_wl_tree(ubi, e1, &ubi->used);
		rb_erase(&e1->u.rb, &ubi->used);
		dbg_wl("anchor-move PEB %d to PEB %d", e1->pnum, e2->pnum);
	} else if (!ubi->scrub.rb_node) {
#else
	if (!ubi->scrub.rb_node) {
#endif
		/*
		 * Now pick the least worn-out used physical eraseblock and a
		 * highly worn-out free physical eraseblock. If the erase
		 * counters differ much enough, start wear-leveling.
		 */
		e1 = rb_entry(rb_first(&ubi->used), struct ubi_wl_entry, u.rb);
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		if (!(e2->ec - e1->ec >= UBI_WL_THRESHOLD)) {
			dbg_wl("no WL needed: min used EC %d, max free EC %d",
			       e1->ec, e2->ec);

			/* Give the unused PEB back */
			wl_tree_add(e2, &ubi->free);
			ubi->free_count++;
			goto out_cancel;
		}
		self_check_in_wl_tree(ubi, e1, &ubi->used);
		rb_erase(&e1->u.rb, &ubi->used);
		dbg_wl("move PEB %d EC %d to PEB %d EC %d",
		       e1->pnum, e1->ec, e2->pnum, e2->ec);
	} else {
		/* Perform scrubbing */
		scrubbing = 1;
		e1 = rb_entry(rb_first(&ubi->scrub), struct ubi_wl_entry, u.rb);
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		self_check_in_wl_tree(ubi, e1, &ubi->scrub);
		rb_erase(&e1->u.rb, &ubi->scrub);
		dbg_wl("scrub PEB %d to PEB %d", e1->pnum, e2->pnum);
	}
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	ubi->move_from = e1;
	ubi->move_to = e2;
	spin_unlock(&ubi->wl_lock);

	/*
	 * Now we are going to copy physical eraseblock @e1->pnum to @e2->pnum.
	 * We so far do not know which logical eraseblock our physical
	 * eraseblock (@e1) belongs to. We have to read the volume identifier
	 * header first.
	 *
	 * Note, we are protected from this PEB being unmapped and erased. The
	 * 'ubi_wl_put_peb()' would wait for moving to be finished if the PEB
	 * which is being moved was unmapped.
	 */
<<<<<<< HEAD
	/*ubi_err("7");*/
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	err = ubi_io_read_vid_hdr(ubi, e1->pnum, vid_hdr, 0);
	if (err && err != UBI_IO_BITFLIPS) {
		if (err == UBI_IO_FF) {
			/*
			 * We are trying to move PEB without a VID header. UBI
			 * always write VID headers shortly after the PEB was
			 * given, so we have a situation when it has not yet
			 * had a chance to write it, because it was preempted.
			 * So add this PEB to the protection queue so far,
			 * because presumably more data will be written there
			 * (including the missing VID header), and then we'll
			 * move it.
			 */
			dbg_wl("PEB %d has no VID header", e1->pnum);
			protect = 1;
<<<<<<< HEAD
			erase_e2 = 0;
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			goto out_not_moved;
		} else if (err == UBI_IO_FF_BITFLIPS) {
			/*
			 * The same situation as %UBI_IO_FF, but bit-flips were
			 * detected. It is better to schedule this PEB for
			 * scrubbing.
			 */
			dbg_wl("PEB %d has no VID header but has bit-flips",
			       e1->pnum);
			scrubbing = 1;
<<<<<<< HEAD
			erase_e2 = 0;
=======
			goto out_not_moved;
		} else if (ubi->fast_attach && err == UBI_IO_BAD_HDR_EBADMSG) {
			/*
			 * While a full scan would detect interrupted erasures
			 * at attach time we can face them here when attached from
			 * Fastmap.
			 */
			dbg_wl("PEB %d has ECC errors, maybe from an interrupted erasure",
			       e1->pnum);
			erase = 1;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			goto out_not_moved;
		}

		ubi_err("error %d while reading VID header from PEB %d",
			err, e1->pnum);
		goto out_error;
	}
<<<<<<< HEAD
	/*	ubi_err("8");*/
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	vol_id = be32_to_cpu(vid_hdr->vol_id);
	lnum = be32_to_cpu(vid_hdr->lnum);

<<<<<<< HEAD
	ubi_msg("[Bean]copy leb from %d to %d tlc(%d)", e1->pnum, e2->pnum, e2->tlc);
	if (e2->tlc)
		err = ubi_eba_copy_tlc_leb(ubi, e1->pnum, e2->pnum, vid_hdr, do_wl); /*MTK: pass do_wl*/
	else
		err = ubi_eba_copy_leb(ubi, e1->pnum, e2->pnum, vid_hdr, do_wl);
	/*ubi_err("err %d", err);*/
=======
	err = ubi_eba_copy_leb(ubi, e1->pnum, e2->pnum, vid_hdr);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (err) {
		if (err == MOVE_CANCEL_RACE) {
			/*
			 * The LEB has not been moved because the volume is
			 * being deleted or the PEB has been put meanwhile. We
			 * should prevent this PEB from being selected for
			 * wear-leveling movement again, so put it to the
			 * protection queue.
			 */
			protect = 1;
<<<<<<< HEAD
			erase_e2 = 0;
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			goto out_not_moved;
		}
		if (err == MOVE_RETRY) {
			scrubbing = 1;
<<<<<<< HEAD
			atomic_inc(&ubi->move_retry); /*MTK*/
			erase_e2 = 0;
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			goto out_not_moved;
		}
		if (err == MOVE_TARGET_BITFLIPS || err == MOVE_TARGET_WR_ERR ||
		    err == MOVE_TARGET_RD_ERR) {
			/*
			 * Target PEB had bit-flips or write error - torture it.
			 */
<<<<<<< HEAD
			torture = 1;  /* need or not ? */
=======
			torture = 1;
			keep = 1;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			goto out_not_moved;
		}

		if (err == MOVE_SOURCE_RD_ERR) {
			/*
			 * An error happened while reading the source PEB. Do
			 * not switch to R/O mode in this case, and give the
			 * upper layers a possibility to recover from this,
			 * e.g. by unmapping corresponding LEB. Instead, just
			 * put this PEB to the @ubi->erroneous list to prevent
			 * UBI from trying to move it over and over again.
			 */
			if (ubi->erroneous_peb_count > ubi->max_erroneous) {
				ubi_err("too many erroneous eraseblocks (%d)",
					ubi->erroneous_peb_count);
				goto out_error;
			}
			erroneous = 1;
			goto out_not_moved;
		}

		if (err < 0)
			goto out_error;

		ubi_assert(0);
	}

	/* The PEB has been successfully moved */
	if (scrubbing)
<<<<<<< HEAD
		dbg_wl("scrubbed PEB %d (LEB %d:%d), data moved to PEB %d",
=======
		ubi_msg("scrubbed PEB %d (LEB %d:%d), data moved to PEB %d",
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			e1->pnum, vol_id, lnum, e2->pnum);
	ubi_free_vid_hdr(ubi, vid_hdr);

	spin_lock(&ubi->wl_lock);
	if (!ubi->move_to_put) {
<<<<<<< HEAD
		ubi_msg("add e2 to used list PEB %d EC %d tlc %d", e2->pnum, e2->ec, e2->tlc);
		if (e2->tlc)
			wl_tree_add(e2, &ubi->tlc_used);
		else
			wl_tree_add(e2, &ubi->used);
=======
		wl_tree_add(e2, &ubi->used);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		e2 = NULL;
	}
	ubi->move_from = ubi->move_to = NULL;
	ubi->move_to_put = ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);

	err = do_sync_erase(ubi, e1, vol_id, lnum, 0);
	if (err) {
		if (e2)
			kmem_cache_free(ubi_wl_entry_slab, e2);
		goto out_ro;
	}

	if (e2) {
		/*
		 * Well, the target PEB was put meanwhile, schedule it for
		 * erasure.
		 */
		dbg_wl("PEB %d (LEB %d:%d) was put meanwhile, erase",
		       e2->pnum, vol_id, lnum);
<<<<<<< HEAD

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		err = do_sync_erase(ubi, e2, vol_id, lnum, 0);
		if (err)
			goto out_ro;
	}

	dbg_wl("done");
	mutex_unlock(&ubi->move_mutex);
	return 0;

	/*
	 * For some reasons the LEB was not moved, might be an error, might be
	 * something else. @e1 was not changed, so return it back. @e2 might
	 * have been changed, schedule it for erasure.
	 */
out_not_moved:
	if (vol_id != -1)
		dbg_wl("cancel moving PEB %d (LEB %d:%d) to PEB %d (%d)",
		       e1->pnum, vol_id, lnum, e2->pnum, err);
	else
		dbg_wl("cancel moving PEB %d to PEB %d (%d)",
		       e1->pnum, e2->pnum, err);
	spin_lock(&ubi->wl_lock);
	if (protect)
		prot_queue_add(ubi, e1);
	else if (erroneous) {
		wl_tree_add(e1, &ubi->erroneous);
		ubi->erroneous_peb_count += 1;
	} else if (scrubbing)
		wl_tree_add(e1, &ubi->scrub);
<<<<<<< HEAD
	else if (archiving)
		wl_tree_add(e1, &ubi->archive);
	else {
		if (e1->tlc)
			wl_tree_add(e1, &ubi->tlc_used);
		else
			wl_tree_add(e1, &ubi->used);
	}
=======
	else if (keep)
		wl_tree_add(e1, &ubi->used);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	ubi_assert(!ubi->move_to_put);
	ubi->move_from = ubi->move_to = NULL;
	ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);

	ubi_free_vid_hdr(ubi, vid_hdr);
<<<<<<< HEAD
	if (erase_e2 == 1) {
		err = do_sync_erase(ubi, e2, vol_id, lnum, torture);
		if (err) {
			kmem_cache_free(ubi_wl_entry_slab, e2);
			goto out_ro;
		}

	} else if (e2->tlc) {
		spin_lock(&ubi->wl_lock);
		wl_tree_add(e2, &ubi->tlc_free);
		ubi->tlc_free_count++;
		spin_unlock(&ubi->wl_lock);
	} else {
		spin_lock(&ubi->wl_lock);
		wl_tree_add(e2, &ubi->free);
		spin_unlock(&ubi->wl_lock);
=======
	err = do_sync_erase(ubi, e2, vol_id, lnum, torture);
	if (err)
		goto out_ro;

	if (erase) {
		err = do_sync_erase(ubi, e1, vol_id, lnum, 1);
		if (err)
			goto out_ro;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}

	mutex_unlock(&ubi->move_mutex);
	return 0;

out_error:
	if (vol_id != -1)
		ubi_err("error %d while moving PEB %d to PEB %d",
			err, e1->pnum, e2->pnum);
	else
		ubi_err("error %d while moving PEB %d (LEB %d:%d) to PEB %d",
			err, e1->pnum, vol_id, lnum, e2->pnum);
	spin_lock(&ubi->wl_lock);
	ubi->move_from = ubi->move_to = NULL;
	ubi->move_to_put = ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);

	ubi_free_vid_hdr(ubi, vid_hdr);
	kmem_cache_free(ubi_wl_entry_slab, e1);
	kmem_cache_free(ubi_wl_entry_slab, e2);

out_ro:
	ubi_ro_mode(ubi);
	mutex_unlock(&ubi->move_mutex);
	ubi_assert(err != 0);
	return err < 0 ? err : -EIO;

out_cancel:
	ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);
	mutex_unlock(&ubi->move_mutex);
	ubi_free_vid_hdr(ubi, vid_hdr);
	return 0;
}
<<<<<<< HEAD
#else /* CONFIG_MTK_SLC_BUFFER_SUPPORT */
static int wear_leveling_worker(struct ubi_device *ubi, struct ubi_work *wrk,
				int shutdown)
{
	int erase_e2 = 1, err, scrubbing = 0, torture = 0, protect = 0, erroneous = 0;
	int vol_id = -1, lnum = -1;
#ifdef CONFIG_MTD_UBI_FASTMAP
	int anchor = wrk->anchor;
#endif
	struct ubi_wl_entry *e1, *e2;
	struct ubi_vid_hdr *vid_hdr;
	int do_wl = 0; /*MTK:wl or not, 1 for wl, 2 for scrubbing*/

	kfree(wrk);
	if (shutdown)
		return 0;

	vid_hdr = ubi_zalloc_vid_hdr(ubi, GFP_NOFS);
	if (!vid_hdr)
		return -ENOMEM;

	mutex_lock(&ubi->move_mutex);
	spin_lock(&ubi->wl_lock);
	ubi_assert(!ubi->move_from && !ubi->move_to);
	ubi_assert(!ubi->move_to_put);

	if (!ubi->free.rb_node ||
	    (!ubi->used.rb_node && !ubi->scrub.rb_node)) {
		/*
		 * No free physical eraseblocks? Well, they must be waiting in
		 * the queue to be erased. Cancel movement - it will be
		 * triggered again when a free physical eraseblock appears.
		 *
		 * No used physical eraseblocks? They must be temporarily
		 * protected from being moved. They will be moved to the
		 * @ubi->used tree later and the wear-leveling will be
		 * triggered again.
		 */
		dbg_wl("cancel WL, a list is empty: free %d, used %d",
		       !ubi->free.rb_node, !ubi->used.rb_node);
		goto out_cancel;
	}

#ifdef CONFIG_MTD_UBI_FASTMAP
	/* Check whether we need to produce an anchor PEB */
	if (!anchor)
		anchor = !anchor_pebs_avalible(&ubi->free);

	if (anchor) {
		e1 = find_anchor_wl_entry(&ubi->used);
		if (!e1)
			goto out_cancel;
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		self_check_in_wl_tree(ubi, e1, &ubi->used);
		rb_erase(&e1->u.rb, &ubi->used);
		dbg_wl("anchor-move PEB %d to PEB %d", e1->pnum, e2->pnum);
	} else if (!ubi->scrub.rb_node) {
#else
	if (!ubi->scrub.rb_node) {
#endif
		/*
		 * Now pick the least worn-out used physical eraseblock and a
		 * highly worn-out free physical eraseblock. If the erase
		 * counters differ much enough, start wear-leveling.
		 */
		e1 = rb_entry(rb_first(&ubi->used), struct ubi_wl_entry, u.rb);
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		if (!(e2->ec - e1->ec >= ubi->wl_th)) {
			dbg_wl("no WL needed: min used EC %d, max free EC %d",
			       e1->ec, e2->ec);

			/* Give the unused PEB back */
			wl_tree_add(e2, &ubi->free);
			ubi->free_count++;
			goto out_cancel;
		}
		self_check_in_wl_tree(ubi, e1, &ubi->used);
		rb_erase(&e1->u.rb, &ubi->used);
		dbg_wl("move PEB %d EC %d to PEB %d EC %d",
		       e1->pnum, e1->ec, e2->pnum, e2->ec);
		do_wl = 1;                /*MTK*/
	} else {
		/* Perform scrubbing */
		scrubbing = 1;
		e1 = rb_entry(rb_first(&ubi->scrub), struct ubi_wl_entry, u.rb);
		e2 = get_peb_for_wl(ubi);
		if (!e2)
			goto out_cancel;

		self_check_in_wl_tree(ubi, e1, &ubi->scrub);
		rb_erase(&e1->u.rb, &ubi->scrub);
		dbg_wl("scrub PEB %d to PEB %d", e1->pnum, e2->pnum);
		do_wl = 2;                /*MTK*/
	}

	ubi->move_from = e1;
	ubi->move_to = e2;
	spin_unlock(&ubi->wl_lock);

	/*
	 * Now we are going to copy physical eraseblock @e1->pnum to @e2->pnum.
	 * We so far do not know which logical eraseblock our physical
	 * eraseblock (@e1) belongs to. We have to read the volume identifier
	 * header first.
	 *
	 * Note, we are protected from this PEB being unmapped and erased. The
	 * 'ubi_wl_put_peb()' would wait for moving to be finished if the PEB
	 * which is being moved was unmapped.
	 */

	err = ubi_io_read_vid_hdr(ubi, e1->pnum, vid_hdr, 0);
	if (err && err != UBI_IO_BITFLIPS) {
		if (err == UBI_IO_FF) {
			/*
			 * We are trying to move PEB without a VID header. UBI
			 * always write VID headers shortly after the PEB was
			 * given, so we have a situation when it has not yet
			 * had a chance to write it, because it was preempted.
			 * So add this PEB to the protection queue so far,
			 * because presumably more data will be written there
			 * (including the missing VID header), and then we'll
			 * move it.
			 */
			dbg_wl("PEB %d has no VID header", e1->pnum);
			protect = 1;
			erase_e2 = 0; /*MTK*/
			goto out_not_moved;
		} else if (err == UBI_IO_FF_BITFLIPS) {
			/*
			 * The same situation as %UBI_IO_FF, but bit-flips were
			 * detected. It is better to schedule this PEB for
			 * scrubbing.
			 */
			dbg_wl("PEB %d has no VID header but has bit-flips",
			       e1->pnum);
			scrubbing = 1;
			erase_e2 = 0; /*MTK*/
			goto out_not_moved;
		}

		ubi_err("error %d while reading VID header from PEB %d",
			err, e1->pnum);
		goto out_error;
	}

	vol_id = be32_to_cpu(vid_hdr->vol_id);
	lnum = be32_to_cpu(vid_hdr->lnum);

	err = ubi_eba_copy_leb(ubi, e1->pnum, e2->pnum, vid_hdr, do_wl); /*MTK: pass do_wl*/
	if (err) {
		if (err == MOVE_CANCEL_RACE) {
			/*
			 * The LEB has not been moved because the volume is
			 * being deleted or the PEB has been put meanwhile. We
			 * should prevent this PEB from being selected for
			 * wear-leveling movement again, so put it to the
			 * protection queue.
			 */
			protect = 1;
			erase_e2 = 0; /*MTK*/
			goto out_not_moved;
		}
		if (err == MOVE_RETRY) {
			scrubbing = 1;
			atomic_inc(&ubi->move_retry); /*MTK*/
			erase_e2 = 0; /*MTK*/
			goto out_not_moved;
		}
		if (err == MOVE_TARGET_BITFLIPS || err == MOVE_TARGET_WR_ERR ||
		    err == MOVE_TARGET_RD_ERR) {
			/*
			 * Target PEB had bit-flips or write error - torture it.
			 */
			torture = 1;
			goto out_not_moved;
		}

		if (err == MOVE_SOURCE_RD_ERR) {
			/*
			 * An error happened while reading the source PEB. Do
			 * not switch to R/O mode in this case, and give the
			 * upper layers a possibility to recover from this,
			 * e.g. by unmapping corresponding LEB. Instead, just
			 * put this PEB to the @ubi->erroneous list to prevent
			 * UBI from trying to move it over and over again.
			 */
			if (ubi->erroneous_peb_count > ubi->max_erroneous) {
				ubi_err("too many erroneous eraseblocks (%d)",
					ubi->erroneous_peb_count);
				goto out_error;
			}
			erroneous = 1;
			goto out_not_moved;
		}

		if (err < 0)
			goto out_error;

		ubi_assert(0);
	}

	/* The PEB has been successfully moved */
	if (scrubbing)
		ubi_msg("scrubbed PEB %d (LEB %d:%d), data moved to PEB %d",
			e1->pnum, vol_id, lnum, e2->pnum);
	ubi_free_vid_hdr(ubi, vid_hdr);

	spin_lock(&ubi->wl_lock);
	if (!ubi->move_to_put) {
		wl_tree_add(e2, &ubi->used);
		e2 = NULL;
	}
	ubi->move_from = ubi->move_to = NULL;
	ubi->move_to_put = ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);

	err = do_sync_erase(ubi, e1, vol_id, lnum, 0);
	if (err) {
		if (e2)
			kmem_cache_free(ubi_wl_entry_slab, e2);
		goto out_ro;
	}

	if (e2) {
		/*
		 * Well, the target PEB was put meanwhile, schedule it for
		 * erasure.
		 */
		dbg_wl("PEB %d (LEB %d:%d) was put meanwhile, erase",
		       e2->pnum, vol_id, lnum);
		err = do_sync_erase(ubi, e2, vol_id, lnum, 0);
		if (err)
			goto out_ro;
	}

	dbg_wl("done");
	mutex_unlock(&ubi->move_mutex);
	return 0;

	/*
	 * For some reasons the LEB was not moved, might be an error, might be
	 * something else. @e1 was not changed, so return it back. @e2 might
	 * have been changed, schedule it for erasure.
	 */
out_not_moved:
	if (vol_id != -1)
		dbg_wl("cancel moving PEB %d (LEB %d:%d) to PEB %d (%d)",
		       e1->pnum, vol_id, lnum, e2->pnum, err);
	else
		dbg_wl("cancel moving PEB %d to PEB %d (%d)",
		       e1->pnum, e2->pnum, err);
	spin_lock(&ubi->wl_lock);
	if (protect)
		prot_queue_add(ubi, e1);
	else if (erroneous) {
		wl_tree_add(e1, &ubi->erroneous);
		ubi->erroneous_peb_count += 1;
	} else if (scrubbing)
		wl_tree_add(e1, &ubi->scrub);
	else
		wl_tree_add(e1, &ubi->used);
	ubi_assert(!ubi->move_to_put);
	ubi->move_from = ubi->move_to = NULL;
	ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);

	ubi_free_vid_hdr(ubi, vid_hdr);
/*MTK start*/
	if (erase_e2 == 1) {
		err = do_sync_erase(ubi, e2, vol_id, lnum, torture);
		if (err) {
			kmem_cache_free(ubi_wl_entry_slab, e2);
			goto out_ro;
		}
	} else {
		spin_lock(&ubi->wl_lock);
		wl_tree_add(e2, &ubi->free);
		spin_unlock(&ubi->wl_lock);
	}
/*MTK end*/
	mutex_unlock(&ubi->move_mutex);
	return 0;

out_error:
	if (vol_id != -1)
		ubi_err("error %d while moving PEB %d to PEB %d",
			err, e1->pnum, e2->pnum);
	else
		ubi_err("error %d while moving PEB %d (LEB %d:%d) to PEB %d",
			err, e1->pnum, vol_id, lnum, e2->pnum);
	spin_lock(&ubi->wl_lock);
	ubi->move_from = ubi->move_to = NULL;
	ubi->move_to_put = ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);

	ubi_free_vid_hdr(ubi, vid_hdr);
	kmem_cache_free(ubi_wl_entry_slab, e1);
	kmem_cache_free(ubi_wl_entry_slab, e2);

out_ro:
	ubi_ro_mode(ubi);
	mutex_unlock(&ubi->move_mutex);
	ubi_assert(err != 0);
	return err < 0 ? err : -EIO;

out_cancel:
	ubi->wl_scheduled = 0;
	spin_unlock(&ubi->wl_lock);
	mutex_unlock(&ubi->move_mutex);
	ubi_free_vid_hdr(ubi, vid_hdr);
	return 0;
}
#endif /* CONFIG_MTK_SLC_BUFFER_SUPPORT */

/**
 * ensure_wear_leveling - schedule wear-leveling if it is needed.
 * @ubi: UBI device description object
 * @nested: set to non-zero if this function is called from UBI worker
 *
 * This function checks if it is time to start wear-leveling and schedules it
 * if yes. This function returns zero in case of success and a negative error
 * code in case of failure.
 */
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
#define SLC_THRESHOLD (5)   /* 1/5=20% */
#define MAX_CHECK_PEBS 100
static int found_full_block(struct ubi_device *ubi, struct rb_root *root)
{
	struct rb_node *p;
	struct ubi_wl_entry *e = NULL;
	int i=0, j, ret;
	char *buf = NULL;
	int *pebs = NULL;
	int peb;

	buf = vmalloc(ubi->min_io_size);
	if (!buf)
		return -1;
	pebs = vzalloc(MAX_CHECK_PEBS*sizeof(int));
	if (!pebs)
		return -1;

	spin_lock(&ubi->wl_lock);
	ubi_rb_for_each_entry(p, e, root, u.rb) {
		pebs[i] = e->pnum;
		i++;
		if(i==MAX_CHECK_PEBS)
			break;
	}
	spin_unlock(&ubi->wl_lock);

	for(i=0;i<MAX_CHECK_PEBS;i++) {
		peb = pebs[i];
		if(peb == 0)
			break;
		ret = ubi_io_read_data(ubi, buf, peb, ubi->leb_size - ubi->min_io_size, ubi->min_io_size);

		if (ret != 0)
			continue;

		for (j = 0; j < ubi->min_io_size; j++) {
			if (((const uint8_t *)buf)[j] != 0xFF) {
				vfree(buf);
				vfree(pebs);
				return peb;
			}
		}
	}
	vfree(buf);
	vfree(pebs);
	return -1;
}
#endif
static int ensure_wear_leveling(struct ubi_device *ubi, int nested)
{
	int err = 0;
	struct ubi_wl_entry *e1;
	struct ubi_wl_entry *e2;
	struct ubi_work *wrk;

	if (ubi->free_count <= ((ubi->peb_count - ubi->mtbl_slots) / SLC_THRESHOLD)) {
		int peb = found_full_block(ubi, &ubi->used);
		if (peb != -1) {
			ubi_err("linger re-schedule peb %d", peb);
			__ubi_wl_archive_leb(ubi, peb);
		}
	}

	spin_lock(&ubi->wl_lock);
	if (ubi->wl_scheduled)
		/* Wear-leveling is already in the work queue */
		goto out_unlock;

	/*
	 * If the ubi->scrub tree is not empty, scrubbing is needed, and the
	 * the WL worker has to be scheduled anyway.
	 */
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	if (!ubi->scrub.rb_node && !ubi->archive.rb_node) {
		if (!ubi->tlc_used.rb_node || !ubi->tlc_free.rb_node)
#else
	if (!ubi->scrub.rb_node) {
		if (!ubi->used.rb_node || !ubi->free.rb_node)
#endif
			/* No physical eraseblocks - no deal */
			goto out_unlock;

		/*
		 * We schedule wear-leveling only if the difference between the
		 * lowest erase counter of used physical eraseblocks and a high
		 * erase counter of free physical eraseblocks is greater than
		 * %UBI_WL_THRESHOLD.
		 */
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		e1 = rb_entry(rb_first(&ubi->tlc_used), struct ubi_wl_entry, u.rb);
		e2 = find_wl_entry(ubi, &ubi->tlc_free, ubi->tlc_wl_th*2);
		if (!(e2->ec - e1->ec >= ubi->tlc_wl_th))
			goto out_unlock;
#else
		e1 = rb_entry(rb_first(&ubi->used), struct ubi_wl_entry, u.rb);
		e2 = find_wl_entry(ubi, &ubi->free, ubi->wl_th*2);

		if (!(e2->ec - e1->ec >= ubi->wl_th))
			goto out_unlock;
#endif
		dbg_wl("schedule wear-leveling");
	} else
		dbg_wl("schedule scrubbing");

	ubi->wl_scheduled = 1;
	spin_unlock(&ubi->wl_lock);

	wrk = kmalloc(sizeof(struct ubi_work), GFP_NOFS);
	if (!wrk) {
		err = -ENOMEM;
		goto out_cancel;
	}

	wrk->anchor = 0;
	wrk->func = &wear_leveling_worker;
	if (nested)
		__schedule_ubi_work(ubi, wrk);
	else
		schedule_ubi_work(ubi, wrk);
	return err;
=======

/**
 * ensure_wear_leveling - schedule wear-leveling if it is needed.
 * @ubi: UBI device description object
 * @nested: set to non-zero if this function is called from UBI worker
 *
 * This function checks if it is time to start wear-leveling and schedules it
 * if yes. This function returns zero in case of success and a negative error
 * code in case of failure.
 */
static int ensure_wear_leveling(struct ubi_device *ubi, int nested)
{
	int err = 0;
	struct ubi_wl_entry *e1;
	struct ubi_wl_entry *e2;
	struct ubi_work *wrk;

	spin_lock(&ubi->wl_lock);
	if (ubi->wl_scheduled)
		/* Wear-leveling is already in the work queue */
		goto out_unlock;

	/*
	 * If the ubi->scrub tree is not empty, scrubbing is needed, and the
	 * the WL worker has to be scheduled anyway.
	 */
	if (!ubi->scrub.rb_node) {
		if (!ubi->used.rb_node || !ubi->free.rb_node)
			/* No physical eraseblocks - no deal */
			goto out_unlock;

		/*
		 * We schedule wear-leveling only if the difference between the
		 * lowest erase counter of used physical eraseblocks and a high
		 * erase counter of free physical eraseblocks is greater than
		 * %UBI_WL_THRESHOLD.
		 */
		e1 = rb_entry(rb_first(&ubi->used), struct ubi_wl_entry, u.rb);
		e2 = find_wl_entry(ubi, &ubi->free, WL_FREE_MAX_DIFF);

		if (!(e2->ec - e1->ec >= UBI_WL_THRESHOLD))
			goto out_unlock;
		dbg_wl("schedule wear-leveling");
	} else
		dbg_wl("schedule scrubbing");

	ubi->wl_scheduled = 1;
	spin_unlock(&ubi->wl_lock);

	wrk = kmalloc(sizeof(struct ubi_work), GFP_NOFS);
	if (!wrk) {
		err = -ENOMEM;
		goto out_cancel;
	}

	wrk->anchor = 0;
	wrk->func = &wear_leveling_worker;
	if (nested)
		__schedule_ubi_work(ubi, wrk);
	else
		schedule_ubi_work(ubi, wrk);
	return err;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

out_cancel:
	spin_lock(&ubi->wl_lock);
	ubi->wl_scheduled = 0;
out_unlock:
	spin_unlock(&ubi->wl_lock);
	return err;
}

#ifdef CONFIG_MTD_UBI_FASTMAP
/**
 * ubi_ensure_anchor_pebs - schedule wear-leveling to produce an anchor PEB.
 * @ubi: UBI device description object
 */
int ubi_ensure_anchor_pebs(struct ubi_device *ubi)
{
	struct ubi_work *wrk;

	spin_lock(&ubi->wl_lock);
	if (ubi->wl_scheduled) {
		spin_unlock(&ubi->wl_lock);
		return 0;
	}
	ubi->wl_scheduled = 1;
	spin_unlock(&ubi->wl_lock);

	wrk = kmalloc(sizeof(struct ubi_work), GFP_NOFS);
	if (!wrk) {
		spin_lock(&ubi->wl_lock);
		ubi->wl_scheduled = 0;
		spin_unlock(&ubi->wl_lock);
		return -ENOMEM;
	}

	wrk->anchor = 1;
	wrk->func = &wear_leveling_worker;
	schedule_ubi_work(ubi, wrk);
	return 0;
}
#endif

/**
 * erase_worker - physical eraseblock erase worker function.
 * @ubi: UBI device description object
 * @wl_wrk: the work object
 * @shutdown: non-zero if the worker has to free memory and exit
<<<<<<< HEAD
 * because the WL sub-system is shutting down
 *
 * This function erases a physical eraseblock and perform torture testing if
 * needed. It also takes care about marking the physical eraseblock bad if
 * needed. Returns zero in case of success and a negative error code in case of
 * failure.
 */
static int erase_worker(struct ubi_device *ubi, struct ubi_work *wl_wrk,
			int shutdown)
{
	struct ubi_wl_entry *e = wl_wrk->e;
	int pnum = e->pnum;
	int vol_id = wl_wrk->vol_id;
	int lnum = wl_wrk->lnum;
	int err, available_consumed = 0;

	if (shutdown) {
		dbg_wl("cancel erasure of PEB %d EC %d", pnum, e->ec);
		kfree(wl_wrk);
		kmem_cache_free(ubi_wl_entry_slab, e);
		return 0;
	}

	dbg_wl("erase PEB %d EC %d LEB %d:%d",
	       pnum, e->ec, wl_wrk->vol_id, wl_wrk->lnum);

	ubi_assert(!ubi_is_fm_block(ubi, e->pnum));

	err = sync_erase(ubi, e, wl_wrk->torture);
	if (!err) {
		/* Fine, we've erased it successfully */
		kfree(wl_wrk);

		spin_lock(&ubi->wl_lock);
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		/*if(ubi_peb_istlc(ubi, e->pnum))*/
		if (e->tlc) {
			wl_tree_add(e, &ubi->tlc_free);
			ubi->tlc_free_count++;
			/*pr_err("\n[ERASE TLC] free count %d\n", ubi->tlc_free_count);*/
		} else
#endif
		{ /* for SLC block erase worker */
			wl_tree_add(e, &ubi->free);
			ubi->free_count++;
		}
		spin_unlock(&ubi->wl_lock);

		/*
		 * One more erase operation has happened, take care about
		 * protected physical eraseblocks.
		 */
		serve_prot_queue(ubi);

		/* And take care about wear-leveling */
		err = ensure_wear_leveling(ubi, 1);
		return err;
	}

	ubi_err("failed to erase PEB %d, error %d", pnum, err);
	kfree(wl_wrk);

	if (err == -EINTR || err == -ENOMEM || err == -EAGAIN ||
	    err == -EBUSY) {
		int err1;

		/* Re-schedule the LEB for erasure */
		err1 = schedule_erase(ubi, e, vol_id, lnum, 0);
		if (err1) {
			err = err1;
			goto out_ro;
		}
		return err;
	}

	kmem_cache_free(ubi_wl_entry_slab, e);
	if (err != -EIO)
		/*
		 * If this is not %-EIO, we have no idea what to do. Scheduling
		 * this physical eraseblock for erasure again would cause
		 * errors again and again. Well, lets switch to R/O mode.
		 */
		goto out_ro;

	/* It is %-EIO, the PEB went bad */

	if (!ubi->bad_allowed) {
		ubi_err("bad physical eraseblock %d detected", pnum);
		goto out_ro;
	}

	spin_lock(&ubi->volumes_lock);
	if (ubi->beb_rsvd_pebs == 0) {
		if (ubi->avail_pebs == 0) {
			spin_unlock(&ubi->volumes_lock);
			ubi_err("no reserved/available physical eraseblocks");
			goto out_ro;
		}
		ubi->avail_pebs -= 1;
		available_consumed = 1;
	}
	spin_unlock(&ubi->volumes_lock);

	ubi_msg("mark PEB %d as bad", pnum);
	err = ubi_io_mark_bad(ubi, pnum);
	if (err)
		goto out_ro;

	spin_lock(&ubi->volumes_lock);
	if (ubi->beb_rsvd_pebs > 0) {
		if (available_consumed) {
			/*
			 * The amount of reserved PEBs increased since we last
			 * checked.
			 */
			ubi->avail_pebs += 1;
			available_consumed = 0;
		}
		ubi->beb_rsvd_pebs -= 1;
	}
	ubi->bad_peb_count += 1;
	ubi->good_peb_count -= 1;
	ubi_calculate_reserved(ubi);
	if (available_consumed)
		ubi_warn("no PEBs in the reserved pool, used an available PEB");
	else if (ubi->beb_rsvd_pebs)
		ubi_msg("%d PEBs left in the reserve", ubi->beb_rsvd_pebs);
	else
		ubi_warn("last PEB from the reserve was used");
	spin_unlock(&ubi->volumes_lock);

	return err;

out_ro:
	if (available_consumed) {
		spin_lock(&ubi->volumes_lock);
		ubi->avail_pebs += 1;
		spin_unlock(&ubi->volumes_lock);
	}
	ubi_ro_mode(ubi);
	return err;
}

/**
 * ubifs_erase_peb - erase physical eraseblock for mtk.
 * @ubi: UBI device description object
 * @wl_wrk: the work object
 * @cancel: non-zero if the worker has to free memory and exit
=======
 * because the WL sub-system is shutting down
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
 *
 * This function erases a physical eraseblock and perform torture testing if
 * needed. It also takes care about marking the physical eraseblock bad if
 * needed. Returns zero in case of success and a negative error code in case of
 * failure.
 */
<<<<<<< HEAD
static int ubi_erase_peb(struct ubi_device *ubi, struct ubi_wl_entry *e,
			  int torture)
{
	int pnum = e->pnum, err, need;
	int retry = 0;

retry_erase:
	retry++;

	err = sync_erase(ubi, e, torture);
	if (!err) {
		/* Fine, we've erased it successfully */
		spin_lock(&ubi->wl_lock);
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		if (e->tlc) {
			wl_tree_add(e, &ubi->tlc_free);
			ubi->tlc_free_count++;
		} else
#endif
		{ /* for SLC block erase worker */
			wl_tree_add(e, &ubi->free);
			ubi->free_count++;
		}
=======
static int erase_worker(struct ubi_device *ubi, struct ubi_work *wl_wrk,
			int shutdown)
{
	struct ubi_wl_entry *e = wl_wrk->e;
	int pnum = e->pnum;
	int vol_id = wl_wrk->vol_id;
	int lnum = wl_wrk->lnum;
	int err, available_consumed = 0;

	if (shutdown) {
		dbg_wl("cancel erasure of PEB %d EC %d", pnum, e->ec);
		kfree(wl_wrk);
		kmem_cache_free(ubi_wl_entry_slab, e);
		return 0;
	}

	dbg_wl("erase PEB %d EC %d LEB %d:%d",
	       pnum, e->ec, wl_wrk->vol_id, wl_wrk->lnum);

	ubi_assert(!ubi_is_fm_block(ubi, e->pnum));

	err = sync_erase(ubi, e, wl_wrk->torture);
	if (!err) {
		/* Fine, we've erased it successfully */
		kfree(wl_wrk);

		spin_lock(&ubi->wl_lock);
		wl_tree_add(e, &ubi->free);
		ubi->free_count++;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		spin_unlock(&ubi->wl_lock);

		/*
		 * One more erase operation has happened, take care about
		 * protected physical eraseblocks.
		 */
		serve_prot_queue(ubi);

		/* And take care about wear-leveling */
		err = ensure_wear_leveling(ubi, 1);
		return err;
	}

	ubi_err("failed to erase PEB %d, error %d", pnum, err);
<<<<<<< HEAD

	if (err == -EINTR || err == -ENOMEM || err == -EAGAIN ||
	    err == -EBUSY) {
		if (retry < 4)
			goto retry_erase;
		else
			goto out_ro;
=======
	kfree(wl_wrk);

	if (err == -EINTR || err == -ENOMEM || err == -EAGAIN ||
	    err == -EBUSY) {
		int err1;

		/* Re-schedule the LEB for erasure */
		err1 = schedule_erase(ubi, e, vol_id, lnum, 0);
		if (err1) {
			err = err1;
			goto out_ro;
		}
		return err;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}

	kmem_cache_free(ubi_wl_entry_slab, e);
	if (err != -EIO)
		/*
		 * If this is not %-EIO, we have no idea what to do. Scheduling
		 * this physical eraseblock for erasure again would cause
		 * errors again and again. Well, lets switch to R/O mode.
		 */
		goto out_ro;

	/* It is %-EIO, the PEB went bad */

	if (!ubi->bad_allowed) {
		ubi_err("bad physical eraseblock %d detected", pnum);
		goto out_ro;
	}

	spin_lock(&ubi->volumes_lock);
<<<<<<< HEAD
	need = ubi->beb_rsvd_level - ubi->beb_rsvd_pebs + 1;
	if (need > 0) {
		need = ubi->avail_pebs >= need ? need : ubi->avail_pebs;
		ubi->avail_pebs -= need;
		ubi->rsvd_pebs += need;
		ubi->beb_rsvd_pebs += need;
		if (need > 0)
			ubi_msg("reserve more %d PEBs", need);
	}

	if (ubi->beb_rsvd_pebs == 0) {
		spin_unlock(&ubi->volumes_lock);
		ubi_err("no reserved physical eraseblocks");
		goto out_ro;
=======
	if (ubi->beb_rsvd_pebs == 0) {
		if (ubi->avail_pebs == 0) {
			spin_unlock(&ubi->volumes_lock);
			ubi_err("no reserved/available physical eraseblocks");
			goto out_ro;
		}
		ubi->avail_pebs -= 1;
		available_consumed = 1;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}
	spin_unlock(&ubi->volumes_lock);

	ubi_msg("mark PEB %d as bad", pnum);
	err = ubi_io_mark_bad(ubi, pnum);
	if (err)
		goto out_ro;

	spin_lock(&ubi->volumes_lock);
<<<<<<< HEAD
	ubi->beb_rsvd_pebs -= 1;
	ubi->bad_peb_count += 1;
	ubi->good_peb_count -= 1;
	ubi_calculate_reserved(ubi);
	if (ubi->beb_rsvd_pebs)
		ubi_msg("%d PEBs left in the reserve", ubi->beb_rsvd_pebs);
	else
		ubi_warn("last PEB from the reserved pool was used");
=======
	if (ubi->beb_rsvd_pebs > 0) {
		if (available_consumed) {
			/*
			 * The amount of reserved PEBs increased since we last
			 * checked.
			 */
			ubi->avail_pebs += 1;
			available_consumed = 0;
		}
		ubi->beb_rsvd_pebs -= 1;
	}
	ubi->bad_peb_count += 1;
	ubi->good_peb_count -= 1;
	ubi_calculate_reserved(ubi);
	if (available_consumed)
		ubi_warn("no PEBs in the reserved pool, used an available PEB");
	else if (ubi->beb_rsvd_pebs)
		ubi_msg("%d PEBs left in the reserve", ubi->beb_rsvd_pebs);
	else
		ubi_warn("last PEB from the reserve was used");
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	spin_unlock(&ubi->volumes_lock);

	return err;

out_ro:
<<<<<<< HEAD
=======
	if (available_consumed) {
		spin_lock(&ubi->volumes_lock);
		ubi->avail_pebs += 1;
		spin_unlock(&ubi->volumes_lock);
	}
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	ubi_ro_mode(ubi);
	return err;
}

/**
 * ubi_wl_put_peb - return a PEB to the wear-leveling sub-system.
 * @ubi: UBI device description object
 * @vol_id: the volume ID that last used this PEB
 * @lnum: the last used logical eraseblock number for the PEB
 * @pnum: physical eraseblock to return
 * @torture: if this physical eraseblock has to be tortured
 *
 * This function is called to return physical eraseblock @pnum to the pool of
 * free physical eraseblocks. The @torture flag has to be set if an I/O error
 * occurred to this @pnum and it has to be tested. This function returns zero
 * in case of success, and a negative error code in case of failure.
 */
int ubi_wl_put_peb(struct ubi_device *ubi, int vol_id, int lnum,
		   int pnum, int torture)
{
	int err;
	struct ubi_wl_entry *e;

	dbg_wl("PEB %d", pnum);
	ubi_assert(pnum >= 0);
	ubi_assert(pnum < ubi->peb_count);

retry:
	spin_lock(&ubi->wl_lock);
	e = ubi->lookuptbl[pnum];
	if (e == ubi->move_from) {
		/*
		 * User is putting the physical eraseblock which was selected to
		 * be moved. It will be scheduled for erasure in the
		 * wear-leveling worker.
		 */
		dbg_wl("PEB %d is being moved, wait", pnum);
		spin_unlock(&ubi->wl_lock);

		/* Wait for the WL worker by taking the @ubi->move_mutex */
		mutex_lock(&ubi->move_mutex);
		mutex_unlock(&ubi->move_mutex);
		goto retry;
	} else if (e == ubi->move_to) {
		/*
		 * User is putting the physical eraseblock which was selected
		 * as the target the data is moved to. It may happen if the EBA
		 * sub-system already re-mapped the LEB in 'ubi_eba_copy_leb()'
		 * but the WL sub-system has not put the PEB to the "used" tree
		 * yet, but it is about to do this. So we just set a flag which
		 * will tell the WL worker that the PEB is not needed anymore
		 * and should be scheduled for erasure.
		 */
		dbg_wl("PEB %d is the target of data moving", pnum);
		ubi_assert(!ubi->move_to_put);
		ubi->move_to_put = 1;
		spin_unlock(&ubi->wl_lock);
		return 0;
<<<<<<< HEAD
	}
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	if (in_wl_tree(e, &ubi->tlc_used)) {
		dbg_wl("PEB %d found @tlc_used", e->pnum);
		self_check_in_wl_tree(ubi, e, &ubi->tlc_used);
		rb_erase(&e->u.rb, &ubi->tlc_used);
	} else if (in_wl_tree(e, &ubi->archive)) {
		self_check_in_wl_tree(ubi, e, &ubi->archive);
		rb_erase(&e->u.rb, &ubi->archive);
	} else
#endif
	if (in_wl_tree(e, &ubi->used)) {
		self_check_in_wl_tree(ubi, e, &ubi->used);
		rb_erase(&e->u.rb, &ubi->used);
	} else if (in_wl_tree(e, &ubi->scrub)) {
		self_check_in_wl_tree(ubi, e, &ubi->scrub);
		rb_erase(&e->u.rb, &ubi->scrub);
	} else if (in_wl_tree(e, &ubi->erroneous)) {
		self_check_in_wl_tree(ubi, e, &ubi->erroneous);
		rb_erase(&e->u.rb, &ubi->erroneous);
		ubi->erroneous_peb_count -= 1;
		ubi_assert(ubi->erroneous_peb_count >= 0);
		/* Erroneous PEBs should be tortured */
		torture = 1;
	} else {
		err = prot_queue_del(ubi, e->pnum);
		if (err) {
			ubi_err("PEB %d not found", pnum);
			ubi_ro_mode(ubi);
			spin_unlock(&ubi->wl_lock);
			return err;
=======
	} else {
		if (in_wl_tree(e, &ubi->used)) {
			self_check_in_wl_tree(ubi, e, &ubi->used);
			rb_erase(&e->u.rb, &ubi->used);
		} else if (in_wl_tree(e, &ubi->scrub)) {
			self_check_in_wl_tree(ubi, e, &ubi->scrub);
			rb_erase(&e->u.rb, &ubi->scrub);
		} else if (in_wl_tree(e, &ubi->erroneous)) {
			self_check_in_wl_tree(ubi, e, &ubi->erroneous);
			rb_erase(&e->u.rb, &ubi->erroneous);
			ubi->erroneous_peb_count -= 1;
			ubi_assert(ubi->erroneous_peb_count >= 0);
			/* Erroneous PEBs should be tortured */
			torture = 1;
		} else {
			err = prot_queue_del(ubi, e->pnum);
			if (err) {
				ubi_err("PEB %d not found", pnum);
				ubi_ro_mode(ubi);
				spin_unlock(&ubi->wl_lock);
				return err;
			}
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		}
	}
	spin_unlock(&ubi->wl_lock);

	err = schedule_erase(ubi, e, vol_id, lnum, torture);
	if (err) {
		spin_lock(&ubi->wl_lock);
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		/*if(ubi_peb_istlc(ubi, pnum))*/
		if (e->tlc)
			wl_tree_add(e, &ubi->tlc_used);
		else
#endif
			wl_tree_add(e, &ubi->used);
=======
		wl_tree_add(e, &ubi->used);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		spin_unlock(&ubi->wl_lock);
	}

	return err;
}

/**
 * ubi_wl_scrub_peb - schedule a physical eraseblock for scrubbing.
 * @ubi: UBI device description object
 * @pnum: the physical eraseblock to schedule
 *
 * If a bit-flip in a physical eraseblock is detected, this physical eraseblock
 * needs scrubbing. This function schedules a physical eraseblock for
 * scrubbing which is done in background. This function returns zero in case of
 * success and a negative error code in case of failure.
 */
int ubi_wl_scrub_peb(struct ubi_device *ubi, int pnum)
{
	struct ubi_wl_entry *e;

	ubi_msg("schedule PEB %d for scrubbing", pnum);

retry:
	spin_lock(&ubi->wl_lock);
	e = ubi->lookuptbl[pnum];
	if (e == ubi->move_from || in_wl_tree(e, &ubi->scrub) ||
				   in_wl_tree(e, &ubi->erroneous)) {
		spin_unlock(&ubi->wl_lock);
		return 0;
	}

	if (e == ubi->move_to) {
		/*
		 * This physical eraseblock was used to move data to. The data
		 * was moved but the PEB was not yet inserted to the proper
		 * tree. We should just wait a little and let the WL worker
		 * proceed.
		 */
		spin_unlock(&ubi->wl_lock);
		dbg_wl("the PEB %d is not in proper tree, retry", pnum);
<<<<<<< HEAD
		/* yield(); */
		goto retry;
	}
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	if (in_wl_tree(e, &ubi->tlc_used)) { /* TLC scrub */
		self_check_in_wl_tree(ubi, e, &ubi->tlc_used);
		rb_erase(&e->u.rb, &ubi->tlc_used);
	} else
#endif
=======
		yield();
		goto retry;
	}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (in_wl_tree(e, &ubi->used)) {
		self_check_in_wl_tree(ubi, e, &ubi->used);
		rb_erase(&e->u.rb, &ubi->used);
	} else {
		int err;

		err = prot_queue_del(ubi, e->pnum);
		if (err) {
			ubi_err("PEB %d not found", pnum);
			ubi_ro_mode(ubi);
			spin_unlock(&ubi->wl_lock);
			return err;
		}
	}

	wl_tree_add(e, &ubi->scrub);
	spin_unlock(&ubi->wl_lock);

	/*
	 * Technically scrubbing is the same as wear-leveling, so it is done
	 * by the WL worker.
	 */
	return ensure_wear_leveling(ubi, 0);
}

<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
static int ubi_check_in_pq(const struct ubi_device *ubi,
			    struct ubi_wl_entry *e)
{
	struct ubi_wl_entry *p;
	int i;

	for (i = 0; i < UBI_PROT_QUEUE_LEN; ++i)
		list_for_each_entry(p, &ubi->pq[i], u.list)
			if (p == e)
				return 0;

	ubi_err("self-check failed for PEB %d, EC %d, Protect queue",
		e->pnum, e->ec);
	return -EINVAL;
}

int __ubi_wl_archive_leb(struct ubi_device *ubi, int pnum)
{
	struct ubi_wl_entry *e;
	struct ubi_volume *mtbl_vol;
	int new_pnum;
	int err = 0;

	ubi_msg("schedule PEB %d for archive", pnum);

retry:
	e = ubi->lookuptbl[pnum];
	if (e == ubi->move_from || in_wl_tree(e, &ubi->archive) ||
				   in_wl_tree(e, &ubi->erroneous)) {
		return 0;
	}

	if (e == ubi->move_to) {
		/*
		 * This physical eraseblock was used to move data to. The data
		 * was moved but the PEB was not yet inserted to the proper
		 * tree. We should just wait a little and let the WL worker
		 * proceed.
		 */
		dbg_wl("the PEB %d is not in proper tree, retry", pnum);
		/* yield(); */
		goto retry;
	}
	spin_lock(&ubi->wl_lock);
	if (in_wl_tree(e, &ubi->used)) {
		self_check_in_wl_tree(ubi, e, &ubi->used);
		rb_erase(&e->u.rb, &ubi->used);
	} else {
		err = ubi_check_in_pq(ubi, e);
		if (err == 0) {
			err = prot_queue_del(ubi, e->pnum);
			if (err) {
				ubi_err("PEB %d not found", pnum);
				ubi_ro_mode(ubi);
				goto out_unlock;
			}
		}
	}
	mtbl_vol = ubi->volumes[vol_id2idx(ubi, UBI_MAINTAIN_VOLUME_ID)];
	new_pnum = mtbl_vol->eba_tbl[0];
	if (pnum == new_pnum) {
		prot_queue_add(ubi, e); /* avoid trigger by slc threshold next time */
		ubi_err("volume is mtbl, cancel");
		goto out_unlock;
	}
	wl_tree_add(e, &ubi->archive);

	/*
	 * Technically scrubbing is the same as wear-leveling, so it is done
	 * by the WL worker.
	 */
out_unlock:
	spin_unlock(&ubi->wl_lock);
	return err;
}

int ubi_wl_archive_leb(struct ubi_device *ubi, struct ubi_volume *vol, int lnum)
{
	int pnum = vol->eba_tbl[lnum];
#if 0
	if (pnum % 2) {
		ubi_err("skip for trigger testing, pnum %d", pnum);
		return 0;
	}
#endif
	ubi_msg("[Bean]archive[ peb %d, leb %d ]\n", pnum, lnum);
	return __ubi_wl_archive_leb(ubi, pnum);
}

int ubi_wipe_mtbl_record(struct ubi_device *ubi, int vol_id)
{
	int i, last = -1;

	ubi_msg("ubi_wipe_mtbl_record vol_id %d\n", vol_id);
	for (i = 0; i < ubi->mtbl_slots; i++) {
		if (cpu_to_be32(ubi->mtbl->info[i].vol_id) == vol_id) {
			ubi->mtbl->info[i].vol_id = cpu_to_be32(0);
			ubi->mtbl->info[i].map = cpu_to_be32(0);
			last = i;
		}
	}
	if (last != -1) {
		ubi->mtbl->info[last].map = cpu_to_be32(1);
		ubi_change_mtbl_record(ubi, last, 0, 0, 0);
	}
#ifdef MTK_TMP_DEBUG_LOG
	ubi_err("maintain table info:\n");
	ubi_err("magic: %x\n", ubi->mtbl->magic);
	ubi_err("crc: %x\n", ubi->mtbl->crc);
	ubi_err("peb_count: %x\n", ubi->mtbl->peb_count);
	ubi_err("ec:\n");
	for (i = 0; i < ubi->mtbl_slots; i++) {
		ubi_err("%x:%d:%d ", cpu_to_be32(ubi->mtbl->info[i].ec), cpu_to_be32(ubi->mtbl->info[i].vol_id),
			cpu_to_be32(ubi->mtbl->info[i].map));
		if ((i + 1) % 16 == 0)
			ubi_err("\n");
	}
#endif
	return 0;
}

int ubi_change_mtbl_record(struct ubi_device *ubi, int idx, int ec, int vol_id, int map)
{
	int i, err = 0, update = 0;
	uint32_t crc;
	struct ubi_volume *mtbl_vol;
	int old_ec, pnum, new_pnum;
	struct ubi_wl_entry *e;
	struct ubi_vid_hdr *vid_hdr = NULL;

	mutex_lock(&ubi->mtbl_mutex);
	ubi_assert(idx >= 0 && idx < ubi->mtbl_slots);
	mtbl_vol = ubi->volumes[vol_id2idx(ubi, UBI_MAINTAIN_VOLUME_ID)];

	if (!ubi->mtbl)
		ubi->mtbl = ubi->empty_mtbl_record;

	if (ec == 0 && ubi->mtbl->info[idx].map == cpu_to_be32(map)) {
		ubi_err("data not changed\n");
		goto out_lock;
	}

	if (ubi->mtbl_count >= (ubi->leb_size / ubi->mtbl_size)) {
try_update:
		update =  1;
	}

	if (mtbl_vol->eba_tbl == NULL) {
		ubi_err("eba not init, change memory table only\n");
		old_ec = be32_to_cpu(ubi->mtbl->info[idx].ec);
		ubi->mtbl->info[idx].ec = cpu_to_be32(old_ec + ec);
		ubi->mtbl->info[idx].vol_id = cpu_to_be32(vol_id);
		ubi->mtbl->info[idx].map = cpu_to_be32(map);
		goto out_lock;
	}
	new_pnum = pnum = mtbl_vol->eba_tbl[0];
	if (pnum < 0 || pnum > ubi->peb_count)
		update =  2;

	if (update) {
		vid_hdr = ubi_zalloc_vid_hdr(ubi, GFP_NOFS);
		if (!vid_hdr) {
			err = -ENOMEM;
			goto out_lock;
		}
		vid_hdr->sqnum = cpu_to_be64(ubi_next_sqnum(ubi));
		vid_hdr->vol_id = cpu_to_be32(mtbl_vol->vol_id);
		vid_hdr->lnum = cpu_to_be32(0);
		vid_hdr->compat = 0;
		vid_hdr->data_pad = cpu_to_be32(mtbl_vol->data_pad);
		vid_hdr->vol_type = UBI_VID_DYNAMIC;
		vid_hdr->data_size = cpu_to_be32(ubi->mtbl_size);
		vid_hdr->copy_flag = 1;
	}

	old_ec = be32_to_cpu(ubi->mtbl->info[idx].ec);
	ubi->mtbl->info[idx].ec = cpu_to_be32(old_ec + ec);
	ubi->mtbl->info[idx].vol_id = cpu_to_be32(vol_id);
	ubi->mtbl->info[idx].map = cpu_to_be32(map);
	ubi_msg("change peb(%d)[%d] ec[%d] map[%d] count[%d]\n",
				new_pnum, idx, be32_to_cpu(ubi->mtbl->info[idx].ec), map, ubi->mtbl_count);
	crc = crc32(UBI_CRC32_INIT, ubi->mtbl->info, ubi->mtbl_slots * sizeof(struct ec_map_info));
	ubi->mtbl->crc = cpu_to_be32(crc);
	for (i = 0; i < UBI_MAINTAIN_VOLUME_EBS; i++) {
		if (update) {
			new_pnum = ubi_wl_get_peb(ubi);
			if (new_pnum < 0) {
				ubi_err("can not get new peb for update maintain table, cancel");
				err = 0;
				goto out_free;
			}
			ubi_err("update maintain table(leb change) at PEB(%d/%d)\n", new_pnum, ubi->peb_count);
			ubi->mtbl_count = 0;
			crc = crc32(UBI_CRC32_INIT, ubi->mtbl, ubi->mtbl_size);
			vid_hdr->data_crc = cpu_to_be32(crc);
			err = ubi_io_write_vid_hdr(ubi, new_pnum, vid_hdr);
			if (err) {
				ubi_err("write vid hdr fail at %d", new_pnum);
				goto out_free;
			}
		}
		err = ubi_io_write_data(ubi, ubi->mtbl, new_pnum, ubi->mtbl_count*ubi->mtbl_size, ubi->mtbl_size);
		ubi_msg("maintain table written\n");
		if (err != 0) {
			ubi_err("ubi io write data fail ret(%d)", err);
			goto try_update;
		}

		switch (update) {
		case 1:
			spin_lock(&ubi->wl_lock);
			e = ubi->lookuptbl[pnum];
			if (in_wl_tree(e, &ubi->used)) {
				self_check_in_wl_tree(ubi, e, &ubi->used);
				rb_erase(&e->u.rb, &ubi->used);
			} else {
				dbg_wl("PEB %d not found @used_tree, try prop", e->pnum);
				err = ubi_check_in_pq(ubi, e);
				if (err)
					goto err_exit_chkinpq;
				err = prot_queue_del(ubi, e->pnum);
				if (err) {
					ubi_err("PEB %d not found", pnum);
					ubi_ro_mode(ubi);
					spin_unlock(&ubi->wl_lock);
					goto out_free;
				}
			}
err_exit_chkinpq:
			spin_unlock(&ubi->wl_lock);
			err = sync_erase(ubi, e, 0);
			spin_lock(&ubi->wl_lock);
			if (!err) {
				wl_tree_add(e, &ubi->free);
				ubi->free_count++;
			} else {
				wl_tree_add(e, &ubi->erroneous);
				ubi->erroneous_peb_count += 1;
			}
			spin_unlock(&ubi->wl_lock);
			/* fallthrough */
		case 2:
			down_read(&ubi->fm_sem);
			mtbl_vol->eba_tbl[0] = new_pnum;
			up_read(&ubi->fm_sem);
			break;
		}
		ubi->mtbl_count++;
	}
#ifdef MTK_TMP_DEBUG_LOG
	ubi_err("change maintain table info:\n");
	ubi_err("magic: %x\n", ubi->mtbl->magic);
	ubi_err("crc: %x\n", ubi->mtbl->crc);
	ubi_err("peb_count: %x\n", ubi->mtbl->peb_count);
	ubi_err("ec map info:\n");
	for (i = 0; i < ubi->mtbl_slots; i++) {
		ubi_err("%x[%d] ", be32_to_cpu(ubi->mtbl->info[i].ec), be32_to_cpu(ubi->mtbl->info[i].map));
		if ((i + 1) % 16 == 0)
			ubi_err("\n");
	}
	ubi_err("\n");
#endif

out_free:
	if (vid_hdr)
		ubi_free_vid_hdr(ubi, vid_hdr);
out_lock:
	mutex_unlock(&ubi->mtbl_mutex);
	return err;
}

#endif

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
/**
 * ubi_wl_flush - flush all pending works.
 * @ubi: UBI device description object
 * @vol_id: the volume id to flush for
 * @lnum: the logical eraseblock number to flush for
 *
 * This function executes all pending works for a particular volume id /
 * logical eraseblock number pair. If either value is set to %UBI_ALL, then it
 * acts as a wildcard for all of the corresponding volume numbers or logical
 * eraseblock numbers. It returns zero in case of success and a negative error
 * code in case of failure.
 */
int ubi_wl_flush(struct ubi_device *ubi, int vol_id, int lnum)
{
	int err = 0;
	int found = 1;

	/*
	 * Erase while the pending works queue is not empty, but not more than
	 * the number of currently pending works.
	 */
	dbg_wl("flush pending work for LEB %d:%d (%d pending works)",
	       vol_id, lnum, ubi->works_count);

	while (found) {
		struct ubi_work *wrk, *tmp;
<<<<<<< HEAD

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		found = 0;

		down_read(&ubi->work_sem);
		spin_lock(&ubi->wl_lock);
		list_for_each_entry_safe(wrk, tmp, &ubi->works, list) {
			if ((vol_id == UBI_ALL || wrk->vol_id == vol_id) &&
			    (lnum == UBI_ALL || wrk->lnum == lnum)) {
				list_del(&wrk->list);
				ubi->works_count -= 1;
				ubi_assert(ubi->works_count >= 0);
				spin_unlock(&ubi->wl_lock);

				err = wrk->func(ubi, wrk, 0);
				if (err) {
					up_read(&ubi->work_sem);
					return err;
				}

				spin_lock(&ubi->wl_lock);
				found = 1;
				break;
			}
		}
		spin_unlock(&ubi->wl_lock);
		up_read(&ubi->work_sem);
	}

	/*
	 * Make sure all the works which have been done in parallel are
	 * finished.
	 */
	down_write(&ubi->work_sem);
	up_write(&ubi->work_sem);

	return err;
}

/**
 * tree_destroy - destroy an RB-tree.
 * @root: the root of the tree to destroy
 */
static void tree_destroy(struct rb_root *root)
{
	struct rb_node *rb;
	struct ubi_wl_entry *e;

	rb = root->rb_node;
	while (rb) {
		if (rb->rb_left)
			rb = rb->rb_left;
		else if (rb->rb_right)
			rb = rb->rb_right;
		else {
			e = rb_entry(rb, struct ubi_wl_entry, u.rb);

			rb = rb_parent(rb);
			if (rb) {
				if (rb->rb_left == &e->u.rb)
					rb->rb_left = NULL;
				else
					rb->rb_right = NULL;
			}

			kmem_cache_free(ubi_wl_entry_slab, e);
		}
	}
}

/**
 * ubi_thread - UBI background thread.
 * @u: the UBI device description object pointer
 */
int ubi_thread(void *u)
{
	int failures = 0;
	struct ubi_device *ubi = u;

	ubi_msg("background thread \"%s\" started, PID %d",
		ubi->bgt_name, task_pid_nr(current));

	set_freezable();
	for (;;) {
		int err;

		if (kthread_should_stop())
			break;

		if (try_to_freeze())
			continue;

		spin_lock(&ubi->wl_lock);
		if (list_empty(&ubi->works) || ubi->ro_mode ||
		    !ubi->thread_enabled || ubi_dbg_is_bgt_disabled(ubi)) {
			set_current_state(TASK_INTERRUPTIBLE);
			spin_unlock(&ubi->wl_lock);
<<<<<<< HEAD
=======

			/*
			 * Check kthread_should_stop() after we set the task
			 * state to guarantee that we either see the stop bit
			 * and exit or the task state is reset to runnable such
			 * that it's not scheduled out indefinitely and detects
			 * the stop bit at kthread_should_stop().
			 */
			if (kthread_should_stop()) {
				set_current_state(TASK_RUNNING);
				break;
			}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			schedule();
			continue;
		}
		spin_unlock(&ubi->wl_lock);

		err = do_work(ubi);
		if (err) {
			ubi_err("%s: work failed with error code %d",
				ubi->bgt_name, err);
			if (failures++ > WL_MAX_FAILURES) {
				/*
				 * Too many failures, disable the thread and
				 * switch to read-only mode.
				 */
				ubi_msg("%s: %d consecutive failures",
					ubi->bgt_name, WL_MAX_FAILURES);
				ubi_ro_mode(ubi);
				ubi->thread_enabled = 0;
				continue;
			}
		} else
			failures = 0;

		cond_resched();
	}

	dbg_wl("background thread \"%s\" is killed", ubi->bgt_name);
<<<<<<< HEAD
=======
	ubi->thread_enabled = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return 0;
}

/**
 * shutdown_work - shutdown all pending works.
 * @ubi: UBI device description object
 */
static void shutdown_work(struct ubi_device *ubi)
{
	while (!list_empty(&ubi->works)) {
		struct ubi_work *wrk;

		wrk = list_entry(ubi->works.next, struct ubi_work, list);
		list_del(&wrk->list);
		wrk->func(ubi, wrk, 1);
		ubi->works_count -= 1;
		ubi_assert(ubi->works_count >= 0);
	}
}

/**
 * ubi_wl_init - initialize the WL sub-system using attaching information.
 * @ubi: UBI device description object
 * @ai: attaching information
 *
 * This function returns zero in case of success, and a negative error code in
 * case of failure.
 */
int ubi_wl_init(struct ubi_device *ubi, struct ubi_attach_info *ai)
{
	int err, i, reserved_pebs, found_pebs = 0;
	struct rb_node *rb1, *rb2;
	struct ubi_ainf_volume *av;
	struct ubi_ainf_peb *aeb, *tmp;
	struct ubi_wl_entry *e;

	ubi->used = ubi->erroneous = ubi->free = ubi->scrub = RB_ROOT;
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	ubi->tlc_used = ubi->tlc_free = ubi->archive = RB_ROOT;
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	spin_lock_init(&ubi->wl_lock);
	mutex_init(&ubi->move_mutex);
	init_rwsem(&ubi->work_sem);
	ubi->max_ec = ai->max_ec;
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	ubi->tlc_max_ec = ai->tlc_max_ec;
#endif

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	INIT_LIST_HEAD(&ubi->works);
#ifdef CONFIG_MTD_UBI_FASTMAP
	INIT_WORK(&ubi->fm_work, update_fastmap_work_fn);
#endif

	sprintf(ubi->bgt_name, UBI_BGT_NAME_PATTERN, ubi->ubi_num);

	err = -ENOMEM;
	ubi->lookuptbl = kzalloc(ubi->peb_count * sizeof(void *), GFP_KERNEL);
	if (!ubi->lookuptbl)
		return err;

	for (i = 0; i < UBI_PROT_QUEUE_LEN; i++)
		INIT_LIST_HEAD(&ubi->pq[i]);
	ubi->pq_head = 0;

<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	ubi->tlc_free_count = 0;
#endif
	ubi->free_count = 0;

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	list_for_each_entry_safe(aeb, tmp, &ai->erase, u.list) {
		cond_resched();

		e = kmem_cache_alloc(ubi_wl_entry_slab, GFP_KERNEL);
		if (!e)
			goto out_free;

		e->pnum = aeb->pnum;
		e->ec = aeb->ec;
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		e->tlc = aeb->tlc;
#endif
		ubi_assert(!ubi_is_fm_block(ubi, e->pnum));
		ubi->lookuptbl[e->pnum] = e;
#if 1
		if (!ubi->ro_mode) {
			if (ubi_erase_peb(ubi, e, 0)) {
				kmem_cache_free(ubi_wl_entry_slab, e);
				goto out_free;
			}
		}
#else
=======
		ubi_assert(!ubi_is_fm_block(ubi, e->pnum));
		ubi->lookuptbl[e->pnum] = e;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		if (schedule_erase(ubi, e, aeb->vol_id, aeb->lnum, 0)) {
			kmem_cache_free(ubi_wl_entry_slab, e);
			goto out_free;
		}
<<<<<<< HEAD
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

		found_pebs++;
	}

<<<<<<< HEAD
=======
	ubi->free_count = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	list_for_each_entry(aeb, &ai->free, u.list) {
		cond_resched();

		e = kmem_cache_alloc(ubi_wl_entry_slab, GFP_KERNEL);
<<<<<<< HEAD
		if (!e)
			goto out_free;

		e->pnum = aeb->pnum;
		e->ec = aeb->ec;
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		e->tlc = aeb->tlc;
#endif
		ubi_assert(e->ec >= 0);
		ubi_assert(!ubi_is_fm_block(ubi, e->pnum));
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
		if (aeb->tlc) {
			wl_tree_add(e, &ubi->tlc_free);
			ubi->tlc_free_count++;
		} else
#endif
		{
			wl_tree_add(e, &ubi->free);
			ubi->free_count++;
		}
=======
		if (!e) {
			err = -ENOMEM;
			goto out_free;
		}

		e->pnum = aeb->pnum;
		e->ec = aeb->ec;
		ubi_assert(e->ec >= 0);
		ubi_assert(!ubi_is_fm_block(ubi, e->pnum));

		wl_tree_add(e, &ubi->free);
		ubi->free_count++;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

		ubi->lookuptbl[e->pnum] = e;

		found_pebs++;
	}

	ubi_rb_for_each_entry(rb1, av, &ai->volumes, rb) {
		ubi_rb_for_each_entry(rb2, aeb, &av->root, u.rb) {
			cond_resched();

			e = kmem_cache_alloc(ubi_wl_entry_slab, GFP_KERNEL);
<<<<<<< HEAD
			if (!e)
				goto out_free;

			e->pnum = aeb->pnum;
			e->ec = aeb->ec;
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
			e->tlc = aeb->tlc;
#endif
=======
			if (!e) {
				err = -ENOMEM;
				goto out_free;
			}

			e->pnum = aeb->pnum;
			e->ec = aeb->ec;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			ubi->lookuptbl[e->pnum] = e;

			if (!aeb->scrub) {
				dbg_wl("add PEB %d EC %d to the used tree",
				       e->pnum, e->ec);
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
				if (aeb->tlc) {
					wl_tree_add(e, &ubi->tlc_used);
				} else
#endif
					wl_tree_add(e, &ubi->used);
=======
				wl_tree_add(e, &ubi->used);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			} else {
				dbg_wl("add PEB %d EC %d to the scrub tree",
				       e->pnum, e->ec);
				wl_tree_add(e, &ubi->scrub);
			}

			found_pebs++;
		}
	}

	dbg_wl("found %i PEBs", found_pebs);

	if (ubi->fm)
<<<<<<< HEAD
		ubi_assert(ubi->good_peb_count == found_pebs + ubi->fm->used_blocks);
=======
		ubi_assert(ubi->good_peb_count == \
			   found_pebs + ubi->fm->used_blocks);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	else
		ubi_assert(ubi->good_peb_count == found_pebs);

	reserved_pebs = WL_RESERVED_PEBS;
#ifdef CONFIG_MTD_UBI_FASTMAP
	/* Reserve enough LEBs to store two fastmaps. */
	reserved_pebs += (ubi->fm_size / ubi->leb_size) * 2;
#endif

	if (ubi->avail_pebs < reserved_pebs) {
		ubi_err("no enough physical eraseblocks (%d, need %d)",
			ubi->avail_pebs, reserved_pebs);
		if (ubi->corr_peb_count)
			ubi_err("%d PEBs are corrupted and not used",
				ubi->corr_peb_count);
<<<<<<< HEAD
=======
		err = -ENOSPC;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		goto out_free;
	}
	ubi->avail_pebs -= reserved_pebs;
	ubi->rsvd_pebs += reserved_pebs;

	/* Schedule wear-leveling if needed */
	err = ensure_wear_leveling(ubi, 0);
	if (err)
		goto out_free;

	return 0;

out_free:
	shutdown_work(ubi);
	tree_destroy(&ubi->used);
	tree_destroy(&ubi->free);
	tree_destroy(&ubi->scrub);
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	tree_destroy(&ubi->tlc_used);
	tree_destroy(&ubi->tlc_free);
	tree_destroy(&ubi->archive);
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	kfree(ubi->lookuptbl);
	return err;
}

/**
 * protection_queue_destroy - destroy the protection queue.
 * @ubi: UBI device description object
 */
static void protection_queue_destroy(struct ubi_device *ubi)
{
	int i;
	struct ubi_wl_entry *e, *tmp;

	for (i = 0; i < UBI_PROT_QUEUE_LEN; ++i) {
		list_for_each_entry_safe(e, tmp, &ubi->pq[i], u.list) {
			list_del(&e->u.list);
			kmem_cache_free(ubi_wl_entry_slab, e);
		}
	}
}

/**
 * ubi_wl_close - close the wear-leveling sub-system.
 * @ubi: UBI device description object
 */
void ubi_wl_close(struct ubi_device *ubi)
{
	dbg_wl("close the WL sub-system");
	shutdown_work(ubi);
	protection_queue_destroy(ubi);
	tree_destroy(&ubi->used);
	tree_destroy(&ubi->erroneous);
	tree_destroy(&ubi->free);
	tree_destroy(&ubi->scrub);
<<<<<<< HEAD
#ifdef CONFIG_MTK_SLC_BUFFER_SUPPORT
	tree_destroy(&ubi->tlc_used);
	tree_destroy(&ubi->tlc_free);
	tree_destroy(&ubi->archive);
#endif
	kfree(ubi->lookuptbl);
}

#ifdef CONFIG_MTK_HIBERNATION
void ubi_wl_move_pg_to_used(struct ubi_device *ubi, int pnum)
{
	struct ubi_wl_entry *e;

	e = ubi->lookuptbl[pnum];
	if (in_wl_tree(e, &ubi->used) == 0) {
		prot_queue_del(ubi, e->pnum);
		wl_tree_add(e, &ubi->used);
	}
}
#endif

=======
	kfree(ubi->lookuptbl);
}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
/**
 * self_check_ec - make sure that the erase counter of a PEB is correct.
 * @ubi: UBI device description object
 * @pnum: the physical eraseblock number to check
 * @ec: the erase counter to check
 *
 * This function returns zero if the erase counter of physical eraseblock @pnum
 * is equivalent to @ec, and a negative error code if not or if an error
 * occurred.
 */
static int self_check_ec(struct ubi_device *ubi, int pnum, int ec)
{
	int err;
	long long read_ec;
	struct ubi_ec_hdr *ec_hdr;

	if (!ubi_dbg_chk_gen(ubi))
		return 0;

	ec_hdr = kzalloc(ubi->ec_hdr_alsize, GFP_NOFS);
	if (!ec_hdr)
		return -ENOMEM;

	err = ubi_io_read_ec_hdr(ubi, pnum, ec_hdr, 0);
	if (err && err != UBI_IO_BITFLIPS) {
		/* The header does not have to exist */
		err = 0;
		goto out_free;
	}

	read_ec = be64_to_cpu(ec_hdr->ec);
	if (ec != read_ec && read_ec - ec > 1) {
		ubi_err("self-check failed for PEB %d", pnum);
		ubi_err("read EC is %lld, should be %d", read_ec, ec);
		dump_stack();
		err = 1;
	} else
		err = 0;

out_free:
	kfree(ec_hdr);
	return err;
}

/**
 * self_check_in_wl_tree - check that wear-leveling entry is in WL RB-tree.
 * @ubi: UBI device description object
 * @e: the wear-leveling entry to check
 * @root: the root of the tree
 *
 * This function returns zero if @e is in the @root RB-tree and %-EINVAL if it
 * is not.
 */
static int self_check_in_wl_tree(const struct ubi_device *ubi,
				 struct ubi_wl_entry *e, struct rb_root *root)
{
	if (!ubi_dbg_chk_gen(ubi))
		return 0;

	if (in_wl_tree(e, root))
		return 0;

	ubi_err("self-check failed for PEB %d, EC %d, RB-tree %p ",
		e->pnum, e->ec, root);
	dump_stack();
	return -EINVAL;
}

/**
 * self_check_in_pq - check if wear-leveling entry is in the protection
 *                        queue.
 * @ubi: UBI device description object
 * @e: the wear-leveling entry to check
 *
 * This function returns zero if @e is in @ubi->pq and %-EINVAL if it is not.
 */
static int self_check_in_pq(const struct ubi_device *ubi,
			    struct ubi_wl_entry *e)
{
	struct ubi_wl_entry *p;
	int i;

	if (!ubi_dbg_chk_gen(ubi))
		return 0;

	for (i = 0; i < UBI_PROT_QUEUE_LEN; ++i)
		list_for_each_entry(p, &ubi->pq[i], u.list)
			if (p == e)
				return 0;

	ubi_err("self-check failed for PEB %d, EC %d, Protect queue",
		e->pnum, e->ec);
	dump_stack();
	return -EINVAL;
}
