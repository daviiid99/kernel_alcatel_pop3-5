/* System call table for ia32 emulation. */

#include <linux/linkage.h>
#include <linux/sys.h>
#include <linux/cache.h>
#include <asm/asm-offsets.h>

<<<<<<< HEAD
#define __SYSCALL_I386(nr, sym, compat) extern asmlinkage void compat(void) ;
#include <asm/syscalls_32.h>
#undef __SYSCALL_I386

#define __SYSCALL_I386(nr, sym, compat) [nr] = compat,
=======
#define __SYSCALL_I386(nr, sym, qual) extern asmlinkage void sym(void) ;
#include <asm/syscalls_32.h>
#undef __SYSCALL_I386

#define __SYSCALL_I386(nr, sym, qual) [nr] = sym,
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

typedef void (*sys_call_ptr_t)(void);

extern void compat_ni_syscall(void);

const sys_call_ptr_t ia32_sys_call_table[__NR_ia32_syscall_max+1] = {
	/*
	 * Smells like a compiler bug -- it doesn't work
	 * when the & below is removed.
	 */
	[0 ... __NR_ia32_syscall_max] = &compat_ni_syscall,
#include <asm/syscalls_32.h>
};
