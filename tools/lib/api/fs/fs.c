/* TODO merge/factor in debugfs.c here */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>

#include "debugfs.h"
#include "fs.h"

static const char * const sysfs__fs_known_mountpoints[] = {
	"/sys",
	0,
};

static const char * const procfs__known_mountpoints[] = {
	"/proc",
	0,
};

struct fs {
	const char		*name;
	const char * const	*mounts;
	char			 path[PATH_MAX + 1];
	bool			 found;
	long			 magic;
};

enum {
	FS__SYSFS  = 0,
	FS__PROCFS = 1,
};

static struct fs fs__entries[] = {
	[FS__SYSFS] = {
		.name	= "sysfs",
		.mounts	= sysfs__fs_known_mountpoints,
		.magic	= SYSFS_MAGIC,
	},
	[FS__PROCFS] = {
		.name	= "proc",
		.mounts	= procfs__known_mountpoints,
		.magic	= PROC_SUPER_MAGIC,
	},
};

static bool fs__read_mounts(struct fs *fs)
{
	bool found = false;
	char type[100];
	FILE *fp;

	fp = fopen("/proc/mounts", "r");
	if (fp == NULL)
		return NULL;

	while (!found &&
	       fscanf(fp, "%*s %" STR(PATH_MAX) "s %99s %*s %*d %*d\n",
		      fs->path, type) == 2) {

		if (strcmp(type, fs->name) == 0)
			found = true;
	}

	fclose(fp);
	return fs->found = found;
}

static int fs__valid_mount(const char *fs, long magic)
{
	struct statfs st_fs;

	if (statfs(fs, &st_fs) < 0)
		return -ENOENT;
<<<<<<< HEAD
	else if (st_fs.f_type != magic)
=======
	else if ((long)st_fs.f_type != magic)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		return -ENOENT;

	return 0;
}

static bool fs__check_mounts(struct fs *fs)
{
	const char * const *ptr;

	ptr = fs->mounts;
	while (*ptr) {
		if (fs__valid_mount(*ptr, fs->magic) == 0) {
			fs->found = true;
			strcpy(fs->path, *ptr);
			return true;
		}
		ptr++;
	}

	return false;
}

static void mem_toupper(char *f, size_t len)
{
	while (len) {
		*f = toupper(*f);
		f++;
		len--;
	}
}

/*
 * Check for "NAME_PATH" environment variable to override fs location (for
 * testing). This matches the recommendation in Documentation/sysfs-rules.txt
 * for SYSFS_PATH.
 */
static bool fs__env_override(struct fs *fs)
{
	char *override_path;
	size_t name_len = strlen(fs->name);
	/* name + "_PATH" + '\0' */
	char upper_name[name_len + 5 + 1];
<<<<<<< HEAD
=======

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	memcpy(upper_name, fs->name, name_len);
	mem_toupper(upper_name, name_len);
	strcpy(&upper_name[name_len], "_PATH");

	override_path = getenv(upper_name);
	if (!override_path)
		return false;

	fs->found = true;
<<<<<<< HEAD
	strncpy(fs->path, override_path, sizeof(fs->path));
=======
	strncpy(fs->path, override_path, sizeof(fs->path) - 1);
	fs->path[sizeof(fs->path) - 1] = '\0';
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return true;
}

static const char *fs__get_mountpoint(struct fs *fs)
{
	if (fs__env_override(fs))
		return fs->path;

	if (fs__check_mounts(fs))
		return fs->path;

	if (fs__read_mounts(fs))
		return fs->path;

	return NULL;
}

static const char *fs__mountpoint(int idx)
{
	struct fs *fs = &fs__entries[idx];

	if (fs->found)
		return (const char *)fs->path;

	return fs__get_mountpoint(fs);
}

#define FS__MOUNTPOINT(name, idx)	\
const char *name##__mountpoint(void)	\
{					\
	return fs__mountpoint(idx);	\
}

FS__MOUNTPOINT(sysfs,  FS__SYSFS);
FS__MOUNTPOINT(procfs, FS__PROCFS);
