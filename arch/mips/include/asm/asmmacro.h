/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003 Ralf Baechle
 */
#ifndef _ASM_ASMMACRO_H
#define _ASM_ASMMACRO_H

#include <asm/hazards.h>
#include <asm/asm-offsets.h>
#include <asm/msa.h>

#ifdef CONFIG_32BIT
#include <asm/asmmacro-32.h>
#endif
#ifdef CONFIG_64BIT
#include <asm/asmmacro-64.h>
#endif

#ifdef CONFIG_CPU_MIPSR2
	.macro	local_irq_enable reg=t0
	ei
	irq_enable_hazard
	.endm

	.macro	local_irq_disable reg=t0
	di
	irq_disable_hazard
	.endm
#else
	.macro	local_irq_enable reg=t0
	mfc0	\reg, CP0_STATUS
	ori	\reg, \reg, 1
	mtc0	\reg, CP0_STATUS
	irq_enable_hazard
	.endm

	.macro	local_irq_disable reg=t0
#ifdef CONFIG_PREEMPT
	lw      \reg, TI_PRE_COUNT($28)
	addi    \reg, \reg, 1
	sw      \reg, TI_PRE_COUNT($28)
#endif
	mfc0	\reg, CP0_STATUS
	ori	\reg, \reg, 1
	xori	\reg, \reg, 1
	mtc0	\reg, CP0_STATUS
	irq_disable_hazard
#ifdef CONFIG_PREEMPT
	lw      \reg, TI_PRE_COUNT($28)
	addi    \reg, \reg, -1
	sw      \reg, TI_PRE_COUNT($28)
#endif
	.endm
#endif /* CONFIG_CPU_MIPSR2 */

	.macro	fpu_save_16even thread tmp=t0
	.set	push
	SET_HARDFLOAT
	cfc1	\tmp, fcr31
	sdc1	$f0,  THREAD_FPR0_LS64(\thread)
	sdc1	$f2,  THREAD_FPR2_LS64(\thread)
	sdc1	$f4,  THREAD_FPR4_LS64(\thread)
	sdc1	$f6,  THREAD_FPR6_LS64(\thread)
	sdc1	$f8,  THREAD_FPR8_LS64(\thread)
	sdc1	$f10, THREAD_FPR10_LS64(\thread)
	sdc1	$f12, THREAD_FPR12_LS64(\thread)
	sdc1	$f14, THREAD_FPR14_LS64(\thread)
	sdc1	$f16, THREAD_FPR16_LS64(\thread)
	sdc1	$f18, THREAD_FPR18_LS64(\thread)
	sdc1	$f20, THREAD_FPR20_LS64(\thread)
	sdc1	$f22, THREAD_FPR22_LS64(\thread)
	sdc1	$f24, THREAD_FPR24_LS64(\thread)
	sdc1	$f26, THREAD_FPR26_LS64(\thread)
	sdc1	$f28, THREAD_FPR28_LS64(\thread)
	sdc1	$f30, THREAD_FPR30_LS64(\thread)
	sw	\tmp, THREAD_FCR31(\thread)
	.set	pop
	.endm

	.macro	fpu_save_16odd thread
	.set	push
	.set	mips64r2
	SET_HARDFLOAT
	sdc1	$f1,  THREAD_FPR1_LS64(\thread)
	sdc1	$f3,  THREAD_FPR3_LS64(\thread)
	sdc1	$f5,  THREAD_FPR5_LS64(\thread)
	sdc1	$f7,  THREAD_FPR7_LS64(\thread)
	sdc1	$f9,  THREAD_FPR9_LS64(\thread)
	sdc1	$f11, THREAD_FPR11_LS64(\thread)
	sdc1	$f13, THREAD_FPR13_LS64(\thread)
	sdc1	$f15, THREAD_FPR15_LS64(\thread)
	sdc1	$f17, THREAD_FPR17_LS64(\thread)
	sdc1	$f19, THREAD_FPR19_LS64(\thread)
	sdc1	$f21, THREAD_FPR21_LS64(\thread)
	sdc1	$f23, THREAD_FPR23_LS64(\thread)
	sdc1	$f25, THREAD_FPR25_LS64(\thread)
	sdc1	$f27, THREAD_FPR27_LS64(\thread)
	sdc1	$f29, THREAD_FPR29_LS64(\thread)
	sdc1	$f31, THREAD_FPR31_LS64(\thread)
	.set	pop
	.endm

	.macro	fpu_save_double thread status tmp
#if defined(CONFIG_64BIT) || defined(CONFIG_CPU_MIPS32_R2)
	sll	\tmp, \status, 5
	bgez	\tmp, 10f
	fpu_save_16odd \thread
10:
#endif
	fpu_save_16even \thread \tmp
	.endm

	.macro	fpu_restore_16even thread tmp=t0
	.set	push
	SET_HARDFLOAT
	lw	\tmp, THREAD_FCR31(\thread)
	ldc1	$f0,  THREAD_FPR0_LS64(\thread)
	ldc1	$f2,  THREAD_FPR2_LS64(\thread)
	ldc1	$f4,  THREAD_FPR4_LS64(\thread)
	ldc1	$f6,  THREAD_FPR6_LS64(\thread)
	ldc1	$f8,  THREAD_FPR8_LS64(\thread)
	ldc1	$f10, THREAD_FPR10_LS64(\thread)
	ldc1	$f12, THREAD_FPR12_LS64(\thread)
	ldc1	$f14, THREAD_FPR14_LS64(\thread)
	ldc1	$f16, THREAD_FPR16_LS64(\thread)
	ldc1	$f18, THREAD_FPR18_LS64(\thread)
	ldc1	$f20, THREAD_FPR20_LS64(\thread)
	ldc1	$f22, THREAD_FPR22_LS64(\thread)
	ldc1	$f24, THREAD_FPR24_LS64(\thread)
	ldc1	$f26, THREAD_FPR26_LS64(\thread)
	ldc1	$f28, THREAD_FPR28_LS64(\thread)
	ldc1	$f30, THREAD_FPR30_LS64(\thread)
	ctc1	\tmp, fcr31
<<<<<<< HEAD
=======
	.set	pop
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	.endm

	.macro	fpu_restore_16odd thread
	.set	push
	.set	mips64r2
	SET_HARDFLOAT
	ldc1	$f1,  THREAD_FPR1_LS64(\thread)
	ldc1	$f3,  THREAD_FPR3_LS64(\thread)
	ldc1	$f5,  THREAD_FPR5_LS64(\thread)
	ldc1	$f7,  THREAD_FPR7_LS64(\thread)
	ldc1	$f9,  THREAD_FPR9_LS64(\thread)
	ldc1	$f11, THREAD_FPR11_LS64(\thread)
	ldc1	$f13, THREAD_FPR13_LS64(\thread)
	ldc1	$f15, THREAD_FPR15_LS64(\thread)
	ldc1	$f17, THREAD_FPR17_LS64(\thread)
	ldc1	$f19, THREAD_FPR19_LS64(\thread)
	ldc1	$f21, THREAD_FPR21_LS64(\thread)
	ldc1	$f23, THREAD_FPR23_LS64(\thread)
	ldc1	$f25, THREAD_FPR25_LS64(\thread)
	ldc1	$f27, THREAD_FPR27_LS64(\thread)
	ldc1	$f29, THREAD_FPR29_LS64(\thread)
	ldc1	$f31, THREAD_FPR31_LS64(\thread)
	.set	pop
	.endm

	.macro	fpu_restore_double thread status tmp
#if defined(CONFIG_64BIT) || defined(CONFIG_CPU_MIPS32_R2)
	sll	\tmp, \status, 5
	bgez	\tmp, 10f				# 16 register mode?

	fpu_restore_16odd \thread
10:
#endif
	fpu_restore_16even \thread \tmp
	.endm

#ifdef CONFIG_CPU_MIPSR2
	.macro	_EXT	rd, rs, p, s
	ext	\rd, \rs, \p, \s
	.endm
#else /* !CONFIG_CPU_MIPSR2 */
	.macro	_EXT	rd, rs, p, s
	srl	\rd, \rs, \p
	andi	\rd, \rd, (1 << \s) - 1
	.endm
#endif /* !CONFIG_CPU_MIPSR2 */

/*
 * Temporary until all gas have MT ASE support
 */
	.macro	DMT	reg=0
	.word	0x41600bc1 | (\reg << 16)
	.endm

	.macro	EMT	reg=0
	.word	0x41600be1 | (\reg << 16)
	.endm

	.macro	DVPE	reg=0
	.word	0x41600001 | (\reg << 16)
	.endm

	.macro	EVPE	reg=0
	.word	0x41600021 | (\reg << 16)
	.endm

	.macro	MFTR	rt=0, rd=0, u=0, sel=0
	 .word	0x41000000 | (\rt << 16) | (\rd << 11) | (\u << 5) | (\sel)
	.endm

	.macro	MTTR	rt=0, rd=0, u=0, sel=0
	 .word	0x41800000 | (\rt << 16) | (\rd << 11) | (\u << 5) | (\sel)
	.endm

#ifdef TOOLCHAIN_SUPPORTS_MSA
	.macro	ld_d	wd, off, base
	.set	push
	.set	mips32r2
	.set	msa
	ld.d	$w\wd, \off(\base)
	.set	pop
	.endm

	.macro	st_d	wd, off, base
	.set	push
	.set	mips32r2
	.set	msa
	st.d	$w\wd, \off(\base)
	.set	pop
	.endm

	.macro	copy_u_w	rd, ws, n
	.set	push
	.set	mips32r2
	.set	msa
	copy_u.w \rd, $w\ws[\n]
	.set	pop
	.endm

	.macro	copy_u_d	rd, ws, n
	.set	push
	.set	mips64r2
	.set	msa
	copy_u.d \rd, $w\ws[\n]
	.set	pop
	.endm

	.macro	insert_w	wd, n, rs
	.set	push
	.set	mips32r2
	.set	msa
	insert.w $w\wd[\n], \rs
	.set	pop
	.endm

	.macro	insert_d	wd, n, rs
	.set	push
	.set	mips64r2
	.set	msa
	insert.d $w\wd[\n], \rs
	.set	pop
	.endm
#else

#ifdef CONFIG_CPU_MICROMIPS
#define CFC_MSA_INSN		0x587e0056
#define CTC_MSA_INSN		0x583e0816
#define LDD_MSA_INSN		0x58000837
#define STD_MSA_INSN		0x5800083f
#define COPY_UW_MSA_INSN	0x58f00056
#define COPY_UD_MSA_INSN	0x58f80056
#define INSERT_W_MSA_INSN	0x59300816
#define INSERT_D_MSA_INSN	0x59380816
#else
#define CFC_MSA_INSN		0x787e0059
#define CTC_MSA_INSN		0x783e0819
#define LDD_MSA_INSN		0x78000823
#define STD_MSA_INSN		0x78000827
#define COPY_UW_MSA_INSN	0x78f00059
#define COPY_UD_MSA_INSN	0x78f80059
#define INSERT_W_MSA_INSN	0x79300819
#define INSERT_D_MSA_INSN	0x79380819
#endif

	/*
	 * Temporary until all toolchains in use include MSA support.
	 */
	.macro	cfcmsa	rd, cs
	.set	push
	.set	noat
	SET_HARDFLOAT
	.insn
	.word	CFC_MSA_INSN | (\cs << 11)
	move	\rd, $1
	.set	pop
	.endm

	.macro	ctcmsa	cd, rs
	.set	push
	.set	noat
	SET_HARDFLOAT
	move	$1, \rs
	.word	CTC_MSA_INSN | (\cd << 6)
	.set	pop
	.endm

	.macro	ld_d	wd, off, base
	.set	push
	.set	noat
	SET_HARDFLOAT
	addu	$1, \base, \off
	.word	LDD_MSA_INSN | (\wd << 6)
	.set	pop
	.endm

	.macro	st_d	wd, off, base
	.set	push
	.set	noat
	SET_HARDFLOAT
	addu	$1, \base, \off
	.word	STD_MSA_INSN | (\wd << 6)
	.set	pop
	.endm

	.macro	copy_u_w	rd, ws, n
	.set	push
	.set	noat
	SET_HARDFLOAT
	.insn
	.word	COPY_UW_MSA_INSN | (\n << 16) | (\ws << 11)
	/* move triggers an assembler bug... */
	or	\rd, $1, zero
	.set	pop
	.endm

	.macro	copy_u_d	rd, ws, n
	.set	push
	.set	noat
	SET_HARDFLOAT
	.insn
	.word	COPY_UD_MSA_INSN | (\n << 16) | (\ws << 11)
	/* move triggers an assembler bug... */
	or	\rd, $1, zero
	.set	pop
	.endm

	.macro	insert_w	wd, n, rs
	.set	push
	.set	noat
	SET_HARDFLOAT
	/* move triggers an assembler bug... */
	or	$1, \rs, zero
	.word	INSERT_W_MSA_INSN | (\n << 16) | (\wd << 6)
	.set	pop
	.endm

	.macro	insert_d	wd, n, rs
	.set	push
	.set	noat
	SET_HARDFLOAT
	/* move triggers an assembler bug... */
	or	$1, \rs, zero
	.word	INSERT_D_MSA_INSN | (\n << 16) | (\wd << 6)
	.set	pop
	.endm
#endif

	.macro	msa_save_all	thread
	st_d	0, THREAD_FPR0, \thread
	st_d	1, THREAD_FPR1, \thread
	st_d	2, THREAD_FPR2, \thread
	st_d	3, THREAD_FPR3, \thread
	st_d	4, THREAD_FPR4, \thread
	st_d	5, THREAD_FPR5, \thread
	st_d	6, THREAD_FPR6, \thread
	st_d	7, THREAD_FPR7, \thread
	st_d	8, THREAD_FPR8, \thread
	st_d	9, THREAD_FPR9, \thread
	st_d	10, THREAD_FPR10, \thread
	st_d	11, THREAD_FPR11, \thread
	st_d	12, THREAD_FPR12, \thread
	st_d	13, THREAD_FPR13, \thread
	st_d	14, THREAD_FPR14, \thread
	st_d	15, THREAD_FPR15, \thread
	st_d	16, THREAD_FPR16, \thread
	st_d	17, THREAD_FPR17, \thread
	st_d	18, THREAD_FPR18, \thread
	st_d	19, THREAD_FPR19, \thread
	st_d	20, THREAD_FPR20, \thread
	st_d	21, THREAD_FPR21, \thread
	st_d	22, THREAD_FPR22, \thread
	st_d	23, THREAD_FPR23, \thread
	st_d	24, THREAD_FPR24, \thread
	st_d	25, THREAD_FPR25, \thread
	st_d	26, THREAD_FPR26, \thread
	st_d	27, THREAD_FPR27, \thread
	st_d	28, THREAD_FPR28, \thread
	st_d	29, THREAD_FPR29, \thread
	st_d	30, THREAD_FPR30, \thread
	st_d	31, THREAD_FPR31, \thread
	.set	push
	.set	noat
	SET_HARDFLOAT
	cfcmsa	$1, MSA_CSR
	sw	$1, THREAD_MSA_CSR(\thread)
	.set	pop
	.endm

	.macro	msa_restore_all	thread
	.set	push
	.set	noat
	SET_HARDFLOAT
	lw	$1, THREAD_MSA_CSR(\thread)
	ctcmsa	MSA_CSR, $1
	.set	pop
	ld_d	0, THREAD_FPR0, \thread
	ld_d	1, THREAD_FPR1, \thread
	ld_d	2, THREAD_FPR2, \thread
	ld_d	3, THREAD_FPR3, \thread
	ld_d	4, THREAD_FPR4, \thread
	ld_d	5, THREAD_FPR5, \thread
	ld_d	6, THREAD_FPR6, \thread
	ld_d	7, THREAD_FPR7, \thread
	ld_d	8, THREAD_FPR8, \thread
	ld_d	9, THREAD_FPR9, \thread
	ld_d	10, THREAD_FPR10, \thread
	ld_d	11, THREAD_FPR11, \thread
	ld_d	12, THREAD_FPR12, \thread
	ld_d	13, THREAD_FPR13, \thread
	ld_d	14, THREAD_FPR14, \thread
	ld_d	15, THREAD_FPR15, \thread
	ld_d	16, THREAD_FPR16, \thread
	ld_d	17, THREAD_FPR17, \thread
	ld_d	18, THREAD_FPR18, \thread
	ld_d	19, THREAD_FPR19, \thread
	ld_d	20, THREAD_FPR20, \thread
	ld_d	21, THREAD_FPR21, \thread
	ld_d	22, THREAD_FPR22, \thread
	ld_d	23, THREAD_FPR23, \thread
	ld_d	24, THREAD_FPR24, \thread
	ld_d	25, THREAD_FPR25, \thread
	ld_d	26, THREAD_FPR26, \thread
	ld_d	27, THREAD_FPR27, \thread
	ld_d	28, THREAD_FPR28, \thread
	ld_d	29, THREAD_FPR29, \thread
	ld_d	30, THREAD_FPR30, \thread
	ld_d	31, THREAD_FPR31, \thread
	.endm

	.macro	msa_init_upper wd
#ifdef CONFIG_64BIT
	insert_d \wd, 1
#else
	insert_w \wd, 2
	insert_w \wd, 3
#endif
	.if	31-\wd
	msa_init_upper	(\wd+1)
	.endif
	.endm

	.macro	msa_init_all_upper
	.set	push
	.set	noat
	SET_HARDFLOAT
	not	$1, zero
	msa_init_upper	0
	.set	pop
	.endm

#endif /* _ASM_ASMMACRO_H */
