#include <linux/module.h>
<<<<<<< HEAD
=======
#include <linux/thread_info.h>
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <asm/byteorder.h>
#include <asm/word-at-a-time.h>

#ifdef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
#define IS_UNALIGNED(src, dst)	0
#else
#define IS_UNALIGNED(src, dst)	\
	(((long) dst | (long) src) & (sizeof(long) - 1))
#endif

/*
 * Do a strncpy, return length of string without final '\0'.
 * 'count' is the user-supplied count (return 'count' if we
 * hit it), 'max' is the address space maximum (and we return
 * -EFAULT if we hit it).
 */
<<<<<<< HEAD
static inline long do_strncpy_from_user(char *dst, const char __user *src, long count, unsigned long max)
{
	const struct word_at_a_time constants = WORD_AT_A_TIME_CONSTANTS;
	long res = 0;
=======
static inline long do_strncpy_from_user(char *dst, const char __user *src,
					unsigned long count, unsigned long max)
{
	const struct word_at_a_time constants = WORD_AT_A_TIME_CONSTANTS;
	unsigned long res = 0;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	/*
	 * Truncate 'max' to the user-specified limit, so that
	 * we only have one limit we need to check in the loop
	 */
	if (max > count)
		max = count;

	if (IS_UNALIGNED(src, dst))
		goto byte_at_a_time;

	while (max >= sizeof(unsigned long)) {
		unsigned long c, data;

		/* Fall back to byte-at-a-time if we get a page fault */
<<<<<<< HEAD
		if (unlikely(__get_user(c,(unsigned long __user *)(src+res))))
			break;
=======
		unsafe_get_user(c, (unsigned long __user *)(src+res), byte_at_a_time);

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		*(unsigned long *)(dst+res) = c;
		if (has_zero(c, &data, &constants)) {
			data = prep_zero_mask(c, data, &constants);
			data = create_zero_mask(data);
			return res + find_zero(data);
		}
		res += sizeof(unsigned long);
		max -= sizeof(unsigned long);
	}

byte_at_a_time:
	while (max) {
		char c;

<<<<<<< HEAD
		if (unlikely(__get_user(c,src+res)))
			return -EFAULT;
=======
		unsafe_get_user(c,src+res, efault);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		dst[res] = c;
		if (!c)
			return res;
		res++;
		max--;
	}

	/*
	 * Uhhuh. We hit 'max'. But was that the user-specified maximum
	 * too? If so, that's ok - we got as much as the user asked for.
	 */
	if (res >= count)
		return res;

	/*
	 * Nope: we hit the address space limit, and we still had more
	 * characters the caller would have wanted. That's an EFAULT.
	 */
<<<<<<< HEAD
=======
efault:
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return -EFAULT;
}

/**
 * strncpy_from_user: - Copy a NUL terminated string from userspace.
 * @dst:   Destination address, in kernel space.  This buffer must be at
 *         least @count bytes long.
 * @src:   Source address, in user space.
 * @count: Maximum number of bytes to copy, including the trailing NUL.
 *
 * Copies a NUL-terminated string from userspace to kernel space.
 *
 * On success, returns the length of the string (not including the trailing
 * NUL).
 *
 * If access to userspace fails, returns -EFAULT (some data may have been
 * copied).
 *
 * If @count is smaller than the length of the string, copies @count bytes
 * and returns @count.
 */
long strncpy_from_user(char *dst, const char __user *src, long count)
{
	unsigned long max_addr, src_addr;

	if (unlikely(count <= 0))
		return 0;

	max_addr = user_addr_max();
	src_addr = (unsigned long)src;
	if (likely(src_addr < max_addr)) {
		unsigned long max = max_addr - src_addr;
<<<<<<< HEAD
		return do_strncpy_from_user(dst, src, count, max);
=======
		long retval;

		check_object_size(dst, count, false);
		user_access_begin();
		retval = do_strncpy_from_user(dst, src, count, max);
		user_access_end();
		return retval;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	}
	return -EFAULT;
}
EXPORT_SYMBOL(strncpy_from_user);
