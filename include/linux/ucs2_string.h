#ifndef _LINUX_UCS2_STRING_H_
#define _LINUX_UCS2_STRING_H_

#include <linux/types.h>	/* for size_t */
#include <linux/stddef.h>	/* for NULL */

typedef u16 ucs2_char_t;

unsigned long ucs2_strnlen(const ucs2_char_t *s, size_t maxlength);
unsigned long ucs2_strlen(const ucs2_char_t *s);
unsigned long ucs2_strsize(const ucs2_char_t *data, unsigned long maxlength);
int ucs2_strncmp(const ucs2_char_t *a, const ucs2_char_t *b, size_t len);

<<<<<<< HEAD
=======
unsigned long ucs2_utf8size(const ucs2_char_t *src);
unsigned long ucs2_as_utf8(u8 *dest, const ucs2_char_t *src,
			   unsigned long maxlength);

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif /* _LINUX_UCS2_STRING_H_ */
