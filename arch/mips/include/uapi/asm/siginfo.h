/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1998, 1999, 2001, 2003 Ralf Baechle
 * Copyright (C) 2000, 2001 Silicon Graphics, Inc.
 */
#ifndef _UAPI_ASM_SIGINFO_H
#define _UAPI_ASM_SIGINFO_H


#define __ARCH_SIGEV_PREAMBLE_SIZE (sizeof(long) + 2*sizeof(int))
#undef __ARCH_SI_TRAPNO /* exception code needs to fill this ...  */

#define HAVE_ARCH_SIGINFO_T

/*
 * We duplicate the generic versions - <asm-generic/siginfo.h> is just borked
 * by design ...
 */
#define HAVE_ARCH_COPY_SIGINFO
struct siginfo;

/*
 * Careful to keep union _sifields from shifting ...
 */
#if _MIPS_SZLONG == 32
#define __ARCH_SI_PREAMBLE_SIZE (3 * sizeof(int))
#elif _MIPS_SZLONG == 64
#define __ARCH_SI_PREAMBLE_SIZE (4 * sizeof(int))
#else
#error _MIPS_SZLONG neither 32 nor 64
#endif

#define __ARCH_SIGSYS

#include <asm-generic/siginfo.h>

typedef struct siginfo {
	int si_signo;
	int si_code;
	int si_errno;
	int __pad0[SI_MAX_SIZE / sizeof(int) - SI_PAD_SIZE - 3];

	union {
		int _pad[SI_PAD_SIZE];

		/* kill() */
		struct {
<<<<<<< HEAD
			pid_t _pid;		/* sender's pid */
=======
			__kernel_pid_t _pid;	/* sender's pid */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			__ARCH_SI_UID_T _uid;	/* sender's uid */
		} _kill;

		/* POSIX.1b timers */
		struct {
<<<<<<< HEAD
			timer_t _tid;		/* timer id */
=======
			__kernel_timer_t _tid;	/* timer id */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			int _overrun;		/* overrun count */
			char _pad[sizeof( __ARCH_SI_UID_T) - sizeof(int)];
			sigval_t _sigval;	/* same as below */
			int _sys_private;	/* not to be passed to user */
		} _timer;

		/* POSIX.1b signals */
		struct {
<<<<<<< HEAD
			pid_t _pid;		/* sender's pid */
=======
			__kernel_pid_t _pid;	/* sender's pid */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
			__ARCH_SI_UID_T _uid;	/* sender's uid */
			sigval_t _sigval;
		} _rt;

		/* SIGCHLD */
		struct {
<<<<<<< HEAD
			pid_t _pid;		/* which child */
			__ARCH_SI_UID_T _uid;	/* sender's uid */
			int _status;		/* exit code */
			clock_t _utime;
			clock_t _stime;
=======
			__kernel_pid_t _pid;	/* which child */
			__ARCH_SI_UID_T _uid;	/* sender's uid */
			int _status;		/* exit code */
			__kernel_clock_t _utime;
			__kernel_clock_t _stime;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		} _sigchld;

		/* IRIX SIGCHLD */
		struct {
<<<<<<< HEAD
			pid_t _pid;		/* which child */
			clock_t _utime;
			int _status;		/* exit code */
			clock_t _stime;
=======
			__kernel_pid_t _pid;	/* which child */
			__kernel_clock_t _utime;
			int _status;		/* exit code */
			__kernel_clock_t _stime;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		} _irix_sigchld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void __user *_addr; /* faulting insn/memory ref. */
#ifdef __ARCH_SI_TRAPNO
			int _trapno;	/* TRAP # which caused the signal */
#endif
			short _addr_lsb;
		} _sigfault;

		/* SIGPOLL, SIGXFSZ (To do ...)	 */
		struct {
			__ARCH_SI_BAND_T _band; /* POLL_IN, POLL_OUT, POLL_MSG */
			int _fd;
		} _sigpoll;

		/* SIGSYS */
		struct {
			void __user *_call_addr; /* calling user insn */
			int _syscall;	/* triggering system call number */
			unsigned int _arch;	/* AUDIT_ARCH_* of syscall */
		} _sigsys;
	} _sifields;
} siginfo_t;

/*
 * si_code values
 * Again these have been chosen to be IRIX compatible.
 */
#undef SI_ASYNCIO
#undef SI_TIMER
#undef SI_MESGQ
#define SI_ASYNCIO	-2	/* sent by AIO completion */
#define SI_TIMER __SI_CODE(__SI_TIMER, -3) /* sent by timer expiration */
#define SI_MESGQ __SI_CODE(__SI_MESGQ, -4) /* sent by real time mesq state change */


#endif /* _UAPI_ASM_SIGINFO_H */
