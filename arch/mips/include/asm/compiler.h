/*
 * Copyright (C) 2004, 2007  Maciej W. Rozycki
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef _ASM_COMPILER_H
#define _ASM_COMPILER_H

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define GCC_IMM_ASM() "n"
#define GCC_REG_ACCUM "$0"
#else
#define GCC_IMM_ASM() "rn"
#define GCC_REG_ACCUM "accum"
#endif

<<<<<<< HEAD
=======
#ifdef CONFIG_CPU_MIPSR6
#define MIPS_ISA_LEVEL "mips64r6"
#define MIPS_ISA_ARCH_LEVEL MIPS_ISA_LEVEL
#define MIPS_ISA_LEVEL_RAW mips64r6
#define MIPS_ISA_ARCH_LEVEL_RAW MIPS_ISA_LEVEL_RAW
#else
/* MIPS64 is a superset of MIPS32 */
#define MIPS_ISA_LEVEL "mips64r2"
#define MIPS_ISA_ARCH_LEVEL "arch=r4000"
#define MIPS_ISA_LEVEL_RAW mips64r2
#define MIPS_ISA_ARCH_LEVEL_RAW MIPS_ISA_LEVEL_RAW
#endif /* CONFIG_CPU_MIPSR6 */

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif /* _ASM_COMPILER_H */
