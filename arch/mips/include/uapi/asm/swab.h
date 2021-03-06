/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 99, 2003 by Ralf Baechle
 */
#ifndef _ASM_SWAB_H
#define _ASM_SWAB_H

#include <linux/compiler.h>
#include <linux/types.h>

#define __SWAB_64_THRU_32__

<<<<<<< HEAD
#if (defined(__mips_isa_rev) && (__mips_isa_rev >= 2)) ||		\
    defined(_MIPS_ARCH_LOONGSON3A)
=======
#if !defined(__mips16) &&					\
	((defined(__mips_isa_rev) && (__mips_isa_rev >= 2)) ||	\
	 defined(_MIPS_ARCH_LOONGSON3A))
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

static inline __attribute_const__ __u16 __arch_swab16(__u16 x)
{
	__asm__(
	"	.set	push			\n"
	"	.set	arch=mips32r2		\n"
	"	wsbh	%0, %1			\n"
	"	.set	pop			\n"
	: "=r" (x)
	: "r" (x));

	return x;
}
#define __arch_swab16 __arch_swab16

static inline __attribute_const__ __u32 __arch_swab32(__u32 x)
{
	__asm__(
	"	.set	push			\n"
	"	.set	arch=mips32r2		\n"
	"	wsbh	%0, %1			\n"
	"	rotr	%0, %0, 16		\n"
	"	.set	pop			\n"
	: "=r" (x)
	: "r" (x));

	return x;
}
#define __arch_swab32 __arch_swab32

/*
 * Having already checked for MIPS R2, enable the optimized version for
 * 64-bit kernel on r2 CPUs.
 */
#ifdef __mips64
static inline __attribute_const__ __u64 __arch_swab64(__u64 x)
{
	__asm__(
	"	.set	push			\n"
	"	.set	arch=mips64r2		\n"
	"	dsbh	%0, %1			\n"
	"	dshd	%0, %0			\n"
	"	.set	pop			\n"
	: "=r" (x)
	: "r" (x));

	return x;
}
#define __arch_swab64 __arch_swab64
#endif /* __mips64 */
<<<<<<< HEAD
#endif /* MIPS R2 or newer or Loongson 3A */
=======
#endif /* (not __mips16) and (MIPS R2 or newer or Loongson 3A) */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif /* _ASM_SWAB_H */
