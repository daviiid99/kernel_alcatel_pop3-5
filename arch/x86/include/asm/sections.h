#ifndef _ASM_X86_SECTIONS_H
#define _ASM_X86_SECTIONS_H

#include <asm-generic/sections.h>
#include <asm/uaccess.h>

extern char __brk_base[], __brk_limit[];
extern struct exception_table_entry __stop___ex_table[];

<<<<<<< HEAD
#if defined(CONFIG_X86_64) && defined(CONFIG_DEBUG_RODATA)
=======
#if defined(CONFIG_X86_64)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
extern char __end_rodata_hpage_align[];
#endif

#endif	/* _ASM_X86_SECTIONS_H */
