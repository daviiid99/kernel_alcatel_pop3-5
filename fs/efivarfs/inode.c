/*
 * Copyright (C) 2012 Red Hat, Inc.
 * Copyright (C) 2012 Jeremy Kerr <jeremy.kerr@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/efi.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/slab.h>
<<<<<<< HEAD
=======
#include <linux/kmemleak.h>
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

#include "internal.h"

struct inode *efivarfs_get_inode(struct super_block *sb,
<<<<<<< HEAD
				const struct inode *dir, int mode, dev_t dev)
=======
				const struct inode *dir, int mode,
				dev_t dev, bool is_removable)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	struct inode *inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode->i_mode = mode;
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
<<<<<<< HEAD
=======
		inode->i_flags = is_removable ? 0 : S_IMMUTABLE;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		switch (mode & S_IFMT) {
		case S_IFREG:
			inode->i_fop = &efivarfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &efivarfs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;
			inc_nlink(inode);
			break;
		}
	}
	return inode;
}

/*
 * Return true if 'str' is a valid efivarfs filename of the form,
 *
 *	VariableName-12345678-1234-1234-1234-1234567891bc
 */
bool efivarfs_valid_name(const char *str, int len)
{
	static const char dashes[EFI_VARIABLE_GUID_LEN] = {
		[8] = 1, [13] = 1, [18] = 1, [23] = 1
	};
	const char *s = str + len - EFI_VARIABLE_GUID_LEN;
	int i;

	/*
	 * We need a GUID, plus at least one letter for the variable name,
	 * plus the '-' separator
	 */
	if (len < EFI_VARIABLE_GUID_LEN + 2)
		return false;

	/* GUID must be preceded by a '-' */
	if (*(s - 1) != '-')
		return false;

	/*
	 * Validate that 's' is of the correct format, e.g.
	 *
	 *	12345678-1234-1234-1234-123456789abc
	 */
	for (i = 0; i < EFI_VARIABLE_GUID_LEN; i++) {
		if (dashes[i]) {
			if (*s++ != '-')
				return false;
		} else {
			if (!isxdigit(*s++))
				return false;
		}
	}

	return true;
}

static void efivarfs_hex_to_guid(const char *str, efi_guid_t *guid)
{
	guid->b[0] = hex_to_bin(str[6]) << 4 | hex_to_bin(str[7]);
	guid->b[1] = hex_to_bin(str[4]) << 4 | hex_to_bin(str[5]);
	guid->b[2] = hex_to_bin(str[2]) << 4 | hex_to_bin(str[3]);
	guid->b[3] = hex_to_bin(str[0]) << 4 | hex_to_bin(str[1]);
	guid->b[4] = hex_to_bin(str[11]) << 4 | hex_to_bin(str[12]);
	guid->b[5] = hex_to_bin(str[9]) << 4 | hex_to_bin(str[10]);
	guid->b[6] = hex_to_bin(str[16]) << 4 | hex_to_bin(str[17]);
	guid->b[7] = hex_to_bin(str[14]) << 4 | hex_to_bin(str[15]);
	guid->b[8] = hex_to_bin(str[19]) << 4 | hex_to_bin(str[20]);
	guid->b[9] = hex_to_bin(str[21]) << 4 | hex_to_bin(str[22]);
	guid->b[10] = hex_to_bin(str[24]) << 4 | hex_to_bin(str[25]);
	guid->b[11] = hex_to_bin(str[26]) << 4 | hex_to_bin(str[27]);
	guid->b[12] = hex_to_bin(str[28]) << 4 | hex_to_bin(str[29]);
	guid->b[13] = hex_to_bin(str[30]) << 4 | hex_to_bin(str[31]);
	guid->b[14] = hex_to_bin(str[32]) << 4 | hex_to_bin(str[33]);
	guid->b[15] = hex_to_bin(str[34]) << 4 | hex_to_bin(str[35]);
}

static int efivarfs_create(struct inode *dir, struct dentry *dentry,
			  umode_t mode, bool excl)
{
<<<<<<< HEAD
	struct inode *inode;
	struct efivar_entry *var;
	int namelen, i = 0, err = 0;
=======
	struct inode *inode = NULL;
	struct efivar_entry *var;
	int namelen, i = 0, err = 0;
	bool is_removable = false;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	if (!efivarfs_valid_name(dentry->d_name.name, dentry->d_name.len))
		return -EINVAL;

<<<<<<< HEAD
	inode = efivarfs_get_inode(dir->i_sb, dir, mode, 0);
	if (!inode)
		return -ENOMEM;

	var = kzalloc(sizeof(struct efivar_entry), GFP_KERNEL);
	if (!var) {
		err = -ENOMEM;
		goto out;
	}
=======
	var = kzalloc(sizeof(struct efivar_entry), GFP_KERNEL);
	if (!var)
		return -ENOMEM;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	/* length of the variable name itself: remove GUID and separator */
	namelen = dentry->d_name.len - EFI_VARIABLE_GUID_LEN - 1;

	efivarfs_hex_to_guid(dentry->d_name.name + namelen + 1,
			&var->var.VendorGuid);

<<<<<<< HEAD
=======
	if (efivar_variable_is_removable(var->var.VendorGuid,
					 dentry->d_name.name, namelen))
		is_removable = true;

	inode = efivarfs_get_inode(dir->i_sb, dir, mode, 0, is_removable);
	if (!inode) {
		err = -ENOMEM;
		goto out;
	}

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	for (i = 0; i < namelen; i++)
		var->var.VariableName[i] = dentry->d_name.name[i];

	var->var.VariableName[i] = '\0';

	inode->i_private = var;
<<<<<<< HEAD
=======
	kmemleak_ignore(var);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	efivar_entry_add(var, &efivarfs_list);
	d_instantiate(dentry, inode);
	dget(dentry);
out:
	if (err) {
		kfree(var);
<<<<<<< HEAD
		iput(inode);
=======
		if (inode)
			iput(inode);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}
	return err;
}

static int efivarfs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct efivar_entry *var = dentry->d_inode->i_private;

	if (efivar_entry_delete(var))
		return -EINVAL;

	drop_nlink(dentry->d_inode);
	dput(dentry);
	return 0;
};

const struct inode_operations efivarfs_dir_inode_operations = {
	.lookup = simple_lookup,
	.unlink = efivarfs_unlink,
	.create = efivarfs_create,
};
