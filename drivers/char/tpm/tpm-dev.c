/*
 * Copyright (C) 2004 IBM Corporation
 * Authors:
 * Leendert van Doorn <leendert@watson.ibm.com>
 * Dave Safford <safford@watson.ibm.com>
 * Reiner Sailer <sailer@watson.ibm.com>
 * Kylene Hall <kjhall@us.ibm.com>
 *
 * Copyright (C) 2013 Obsidian Research Corp
 * Jason Gunthorpe <jgunthorpe@obsidianresearch.com>
 *
 * Device file system interface to the TPM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 */
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "tpm.h"

struct file_priv {
	struct tpm_chip *chip;

	/* Data passed to and from the tpm via the read/write calls */
<<<<<<< HEAD
	atomic_t data_pending;
=======
	size_t data_pending;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	struct mutex buffer_mutex;

	struct timer_list user_read_timer;      /* user needs to claim result */
	struct work_struct work;

	u8 data_buffer[TPM_BUFSIZE];
};

static void user_reader_timeout(unsigned long ptr)
{
	struct file_priv *priv = (struct file_priv *)ptr;

	schedule_work(&priv->work);
}

static void timeout_work(struct work_struct *work)
{
	struct file_priv *priv = container_of(work, struct file_priv, work);

	mutex_lock(&priv->buffer_mutex);
<<<<<<< HEAD
	atomic_set(&priv->data_pending, 0);
=======
	priv->data_pending = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	memset(priv->data_buffer, 0, sizeof(priv->data_buffer));
	mutex_unlock(&priv->buffer_mutex);
}

static int tpm_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc = file->private_data;
	struct tpm_chip *chip = container_of(misc, struct tpm_chip,
					     vendor.miscdev);
	struct file_priv *priv;

	/* It's assured that the chip will be opened just once,
	 * by the check of is_open variable, which is protected
	 * by driver_lock. */
	if (test_and_set_bit(0, &chip->is_open)) {
		dev_dbg(chip->dev, "Another process owns this TPM\n");
		return -EBUSY;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		clear_bit(0, &chip->is_open);
		return -ENOMEM;
	}

	priv->chip = chip;
<<<<<<< HEAD
	atomic_set(&priv->data_pending, 0);
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	mutex_init(&priv->buffer_mutex);
	setup_timer(&priv->user_read_timer, user_reader_timeout,
			(unsigned long)priv);
	INIT_WORK(&priv->work, timeout_work);

	file->private_data = priv;
	get_device(chip->dev);
	return 0;
}

static ssize_t tpm_read(struct file *file, char __user *buf,
			size_t size, loff_t *off)
{
	struct file_priv *priv = file->private_data;
<<<<<<< HEAD
	ssize_t ret_size;
=======
	ssize_t ret_size = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	int rc;

	del_singleshot_timer_sync(&priv->user_read_timer);
	flush_work(&priv->work);
<<<<<<< HEAD
	ret_size = atomic_read(&priv->data_pending);
	if (ret_size > 0) {	/* relay data */
		ssize_t orig_ret_size = ret_size;
		if (size < ret_size)
			ret_size = size;

		mutex_lock(&priv->buffer_mutex);
		rc = copy_to_user(buf, priv->data_buffer, ret_size);
		memset(priv->data_buffer, 0, orig_ret_size);
		if (rc)
			ret_size = -EFAULT;

		mutex_unlock(&priv->buffer_mutex);
	}

	atomic_set(&priv->data_pending, 0);

=======
	mutex_lock(&priv->buffer_mutex);

	if (priv->data_pending) {
		ret_size = min_t(ssize_t, size, priv->data_pending);
		rc = copy_to_user(buf, priv->data_buffer, ret_size);
		memset(priv->data_buffer, 0, priv->data_pending);
		if (rc)
			ret_size = -EFAULT;

		priv->data_pending = 0;
	}

	mutex_unlock(&priv->buffer_mutex);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return ret_size;
}

static ssize_t tpm_write(struct file *file, const char __user *buf,
			 size_t size, loff_t *off)
{
	struct file_priv *priv = file->private_data;
	size_t in_size = size;
	ssize_t out_size;

<<<<<<< HEAD
	/* cannot perform a write until the read has cleared
	   either via tpm_read or a user_read_timer timeout.
	   This also prevents splitted buffered writes from blocking here.
	*/
	if (atomic_read(&priv->data_pending) != 0)
		return -EBUSY;

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (in_size > TPM_BUFSIZE)
		return -E2BIG;

	mutex_lock(&priv->buffer_mutex);

<<<<<<< HEAD
=======
	/* Cannot perform a write until the read has cleared either via
	 * tpm_read or a user_read_timer timeout. This also prevents split
	 * buffered writes from blocking here.
	 */
	if (priv->data_pending != 0) {
		mutex_unlock(&priv->buffer_mutex);
		return -EBUSY;
	}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	if (copy_from_user
	    (priv->data_buffer, (void __user *) buf, in_size)) {
		mutex_unlock(&priv->buffer_mutex);
		return -EFAULT;
	}

	/* atomic tpm command send and result receive */
	out_size = tpm_transmit(priv->chip, priv->data_buffer,
				sizeof(priv->data_buffer));
	if (out_size < 0) {
		mutex_unlock(&priv->buffer_mutex);
		return out_size;
	}

<<<<<<< HEAD
	atomic_set(&priv->data_pending, out_size);
=======
	priv->data_pending = out_size;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	mutex_unlock(&priv->buffer_mutex);

	/* Set a timeout by which the reader must come claim the result */
	mod_timer(&priv->user_read_timer, jiffies + (60 * HZ));

	return in_size;
}

/*
 * Called on file close
 */
static int tpm_release(struct inode *inode, struct file *file)
{
	struct file_priv *priv = file->private_data;

	del_singleshot_timer_sync(&priv->user_read_timer);
	flush_work(&priv->work);
	file->private_data = NULL;
<<<<<<< HEAD
	atomic_set(&priv->data_pending, 0);
=======
	priv->data_pending = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	clear_bit(0, &priv->chip->is_open);
	put_device(priv->chip->dev);
	kfree(priv);
	return 0;
}

static const struct file_operations tpm_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.open = tpm_open,
	.read = tpm_read,
	.write = tpm_write,
	.release = tpm_release,
};

int tpm_dev_add_device(struct tpm_chip *chip)
{
	int rc;

	chip->vendor.miscdev.fops = &tpm_fops;
	if (chip->dev_num == 0)
		chip->vendor.miscdev.minor = TPM_MINOR;
	else
		chip->vendor.miscdev.minor = MISC_DYNAMIC_MINOR;

	chip->vendor.miscdev.name = chip->devname;
	chip->vendor.miscdev.parent = chip->dev;

	rc = misc_register(&chip->vendor.miscdev);
	if (rc) {
		chip->vendor.miscdev.name = NULL;
		dev_err(chip->dev,
			"unable to misc_register %s, minor %d err=%d\n",
			chip->vendor.miscdev.name,
			chip->vendor.miscdev.minor, rc);
	}
	return rc;
}

void tpm_dev_del_device(struct tpm_chip *chip)
{
	if (chip->vendor.miscdev.name)
		misc_deregister(&chip->vendor.miscdev);
}
